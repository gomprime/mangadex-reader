#pragma once

#include <memory>

#include <borealis.hpp>

#include "ui/scroll_content_sync.hpp"
#include "util/lifetime.hpp"

namespace ui
{

// "Biblioteca" tab: everything saved locally in library.json (favorites and
// reading progress), sorted by most recently updated. Tapping an entry with
// saved progress jumps straight into the reader at the last read page.
class LibraryTab : public brls::Box
{
  public:
    LibraryTab();
    ~LibraryTab() override;

    void willAppear(bool resetState) override;

    static brls::View* create();

  private:
    void reload();

    brls::ScrollingFrame* scrollingFrame;
    brls::Box* listContainer;
    brls::Label* statusLabel;
    bool contentViewAttached = false;

    std::unique_ptr<ScrollContentSync> scrollSync;

    util::AliveFlag alive = util::makeAliveFlag();
};

} // namespace ui
