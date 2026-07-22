#pragma once

#include <memory>
#include <string>

#include <borealis.hpp>

#include "api/models.hpp"
#include "ui/scroll_content_sync.hpp"
#include "util/lifetime.hpp"

namespace ui
{

// "Buscar" tab: a search button (opens the Switch software keyboard) and a
// paginated, scrollable list of results as MangaCard rows.
class SearchTab : public brls::Box
{
  public:
    SearchTab();
    ~SearchTab() override;

    void willAppear(bool resetState) override;

    static brls::View* create();

  private:
    void openKeyboard();
    void performSearch(const std::string& query, bool reset);
    void appendResults(const api::SearchResult& result);
    void clearResults();
    void setStatus(const std::string& text);

    brls::ScrollingFrame* scrollingFrame;
    brls::Box* resultsContainer;
    brls::Label* statusLabel;
    brls::Button* searchButton;
    brls::Button* loadMoreButton = nullptr;
    bool contentViewAttached = false;

    std::unique_ptr<ScrollContentSync> scrollSync;

    std::string currentQuery;
    int offset = 0;
    int knownTotal = 0;
    static constexpr int kPageSize = 20;

    util::AliveFlag alive = util::makeAliveFlag();
};

} // namespace ui
