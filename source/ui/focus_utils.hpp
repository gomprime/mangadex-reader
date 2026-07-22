#pragma once

#include <borealis.hpp>

namespace ui
{

// True if the currently focused view is container itself or a descendant
// of it. Use this before redirecting focus away from a container you're
// about to clear - only do that if focus is actually about to be destroyed;
// otherwise redirecting unconditionally steals focus from wherever it
// legitimately is (e.g. the sidebar) every time the container reloads.
inline bool isFocusInside(brls::View* container)
{
    brls::View* view = brls::Application::getCurrentFocus();

    while (view)
    {
        if (view == container)
            return true;
        view = view->getParent();
    }

    return false;
}

} // namespace ui
