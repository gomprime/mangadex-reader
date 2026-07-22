#pragma once

#include <functional>

namespace util
{

// Runs fn on a small fixed pool of background threads with a stack large
// enough for TLS handshakes (mbedtls) and HTTP parsing (curl/nlohmann::json).
// The default pthread stack size on devkitA64 is too small for that and
// silently overflows, corrupting memory instead of throwing - use this
// instead of std::thread for anything that touches net::HttpClient.
//
// priority: if true, fn jumps to the front of the queue instead of the back.
// Use this for work the user is actively waiting on right now (the page
// they just turned to) so it isn't stuck behind a backlog of earlier
// speculative work (preloading pages the user has since paged past) that
// hasn't been picked up by a worker yet.
void spawnWorkerThread(std::function<void()> fn, bool priority = false);

// Blocks until every worker thread spawned via spawnWorkerThread() has
// finished, or timeoutMs elapses - whichever comes first. Call this after
// the main loop stops and before tearing down global state that worker
// threads might still be using mid-request (curl_global_cleanup() etc.):
// those threads are detached and untracked otherwise, so quitting while one
// is mid-request races curl's global state and can crash on exit.
void waitForWorkerThreads(int timeoutMs);

} // namespace util
