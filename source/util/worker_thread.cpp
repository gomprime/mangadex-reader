#include "util/worker_thread.hpp"

#include <atomic>
#include <condition_variable>
#include <ctime>
#include <deque>
#include <mutex>
#include <pthread.h>

namespace util
{

namespace
{
    // 1 MiB: comfortable margin for mbedTLS handshakes + curl parsing + json
    // parsing, well above the default pthread stack size on devkitA64.
    constexpr size_t kWorkerStackSize = 1 * 1024 * 1024;

    // Small, fixed pool instead of one detached OS thread per task: a
    // browsing session spawns a *lot* of short-lived tasks (a cover download
    // per manga card, a request per search/page/preload), each of which used
    // to mean creating and tearing down a brand new pthread. A handful of
    // long-lived threads pulling from a queue creates a fixed, small number
    // of OS threads instead. They must still be actually joined before exit
    // though (see waitForWorkerThreads) - the homebrew loader appears not to
    // tolerate the process closing with any of them still alive, even idle.
    constexpr int kPoolSize = 4;

    std::mutex queueMutex;
    std::condition_variable queueCv;
    std::deque<std::function<void()>> taskQueue;
    std::atomic<int> pendingTasks { 0 }; // queued + currently running
    bool shuttingDown = false;
    bool poolStarted = false; // main-thread only, like spawnWorkerThread itself
    pthread_t poolThreads[kPoolSize];
    pthread_once_t poolInitOnce = PTHREAD_ONCE_INIT;

    void* poolWorkerMain(void*)
    {
        while (true)
        {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCv.wait(lock, [] { return shuttingDown || !taskQueue.empty(); });

                // Check shuttingDown *before* taskQueue.empty(): once
                // shutdown is requested, abandon whatever's left in the
                // queue and return immediately instead of draining it task
                // by task. A backlog of stale preload requests (built up
                // from paging through a long chapter faster than 4 workers
                // can keep up) could otherwise take a very long time to
                // work through, during which waitForWorkerThreads()'s
                // pthread_join() blocks the main thread - freezing the
                // whole app, unable to even process the button that
                // triggered the exit in the first place.
                if (shuttingDown)
                    return nullptr;

                if (taskQueue.empty())
                    continue;

                task = std::move(taskQueue.front());
                taskQueue.pop_front();
            }

            task();
            pendingTasks.fetch_sub(1, std::memory_order_release);
        }
    }

    void startPool()
    {
        poolStarted = true;

        for (int i = 0; i < kPoolSize; i++)
        {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, kWorkerStackSize);

            pthread_create(&poolThreads[i], &attr, poolWorkerMain, nullptr);

            pthread_attr_destroy(&attr);
        }
    }
}

void spawnWorkerThread(std::function<void()> fn, bool priority)
{
    pthread_once(&poolInitOnce, startPool);

    pendingTasks.fetch_add(1, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (priority)
            taskQueue.push_front(std::move(fn));
        else
            taskQueue.push_back(std::move(fn));
    }

    queueCv.notify_one();
}

void waitForWorkerThreads(int timeoutMs)
{
    constexpr int kPollMs = 20;
    int waited = 0;

    while (pendingTasks.load(std::memory_order_acquire) > 0 && waited < timeoutMs)
    {
        struct timespec ts { 0, kPollMs * 1000000L };
        nanosleep(&ts, nullptr);
        waited += kPollMs;
    }

    // Waiting for the queue to drain isn't enough on its own - the pool
    // threads themselves loop forever waiting for more work and never
    // terminate otherwise, so they're still alive (just idle) at this
    // point. Tell them to stop and actually join them so zero extra
    // threads are left running once this returns.
    if (!poolStarted)
        return;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        shuttingDown = true;
    }
    queueCv.notify_all();

    for (int i = 0; i < kPoolSize; i++)
        pthread_join(poolThreads[i], nullptr);
}

} // namespace util
