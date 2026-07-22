#include "util/main_thread.hpp"

#include <mutex>
#include <vector>

namespace util
{

namespace
{
    std::mutex queueMutex;
    std::vector<std::function<void()>> queue;
}

void runOnMainThread(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    queue.push_back(std::move(callback));
}

void drainMainThreadQueue()
{
    std::vector<std::function<void()>> pending;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        pending.swap(queue);
    }

    for (auto& callback : pending)
        callback();
}

} // namespace util
