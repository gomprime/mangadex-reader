#include <curl/curl.h>

#include <cstdlib>

#include <borealis.hpp>

#include "storage/paths.hpp"
#include "ui/main_activity.hpp"
#include "util/main_thread.hpp"
#include "util/worker_thread.hpp"

namespace
{
    // Drains callbacks queued by worker threads (network/disk results) onto
    // the main thread once per frame, right before Borealis renders.
    class MainThreadDrainTask : public brls::RepeatingTask
    {
      public:
        MainThreadDrainTask()
            : RepeatingTask(0)
        {
        }

        void run() override
        {
            util::drainMainThreadQueue();
        }
    };
}

int main(int argc, char* argv[])
{
    brls::Logger::setLogLevel(brls::LogLevel::INFO);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    storage::EnsureDirectories();

    if (!brls::Application::init())
    {
        brls::Logger::error("Unable to init Borealis application");
        return EXIT_FAILURE;
    }

    brls::Application::createWindow("MangaDex Reader");
    brls::Application::setGlobalQuit(true);

    MainThreadDrainTask drainTask;
    drainTask.start();

    brls::Application::pushActivity(new ui::MainActivity());

    while (brls::Application::mainLoop())
        ;

    // Worker threads are detached and untracked otherwise - quitting while
    // one is still mid-request (very likely, since covers/search/downloads
    // all run in the background) would race curl_global_cleanup() tearing
    // down state a background thread is still actively using, crashing on
    // exit. 3s is generous; a hung request just gets abandoned past that.
    util::waitForWorkerThreads(3000);

    curl_global_cleanup();

    return EXIT_SUCCESS;
}
