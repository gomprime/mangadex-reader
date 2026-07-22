#pragma once

#include <memory>
#include <string>

#include <borealis.hpp>

#include "api/models.hpp"
#include "ui/scroll_content_sync.hpp"
#include "util/lifetime.hpp"

namespace ui
{

// "Novidades" tab: toggle between latest chapter updates and most popular
// manga, both as a paginated scrollable list of MangaCard rows.
class BrowseTab : public brls::Box
{
  public:
    enum class Mode
    {
        Latest,
        Popular,
    };

    BrowseTab();
    ~BrowseTab() override;

    void willAppear(bool resetState) override;

    static brls::View* create();

  private:
    void switchMode(Mode newMode);
    void loadPage(bool reset);
    void appendResults(const api::SearchResult& result);
    void clearResults();
    void setStatus(const std::string& text);

    brls::Box* toggleRow = nullptr;
    brls::Box* contentArea = nullptr;
    brls::ScrollingFrame* scrollingFrame;
    brls::Box* resultsContainer;
    brls::Label* statusLabel;
    brls::Button* loadMoreButton = nullptr;
    brls::Button* latestButton;
    brls::Button* popularButton;
    bool contentViewAttached = false;

    std::unique_ptr<ScrollContentSync> scrollSync;

    Mode mode = Mode::Latest;
    int offset = 0;
    int knownTotal = 0;
    static constexpr int kPageSize = 20;

    util::AliveFlag alive = util::makeAliveFlag();
};

} // namespace ui
