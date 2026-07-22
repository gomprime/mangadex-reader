#pragma once

#include <functional>

namespace util
{

// Queues a callback to run on the main (UI) thread on the next drain() call.
// Network/decoding work happens on worker threads; anything that touches
// Borealis views must be marshalled back through here.
void runOnMainThread(std::function<void()> callback);

// Drains and executes all pending callbacks. Must only be called from the main thread.
void drainMainThreadQueue();

} // namespace util
