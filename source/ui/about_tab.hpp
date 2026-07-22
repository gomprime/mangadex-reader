#pragma once

#include <borealis.hpp>

namespace ui
{

// "Sobre" tab: app name, version, author and acknowledgments. Static
// content, nothing to load or save.
class AboutTab : public brls::Box
{
  public:
    AboutTab();

    static brls::View* create();
};

} // namespace ui
