#pragma once

#include <atomic>
#include <memory>

namespace util
{

// Views that kick off background downloads need a way to know, once the
// download finishes and the result is marshalled back to the main thread,
// whether they still exist (the user may have navigated away in the meantime).
// Hold an AliveFlag as a member, capture a copy of the shared_ptr in the
// worker lambda, and check it before touching `this` on the main thread.
using AliveFlag = std::shared_ptr<std::atomic<bool>>;

inline AliveFlag makeAliveFlag()
{
    return std::make_shared<std::atomic<bool>>(true);
}

} // namespace util
