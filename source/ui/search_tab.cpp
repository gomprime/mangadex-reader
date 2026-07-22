#include "ui/search_tab.hpp"

#include <borealis/platforms/switch/swkbd.hpp>

#include "api/mangadex_client.hpp"
#include "storage/library_store.hpp"
#include "ui/focus_utils.hpp"
#include "ui/manga_card.hpp"
#include "ui/manga_detail_activity.hpp"
#include "util/main_thread.hpp"
#include "util/worker_thread.hpp"

namespace ui
{

SearchTab::SearchTab()
    : brls::Box(brls::Axis::COLUMN)
{
    this->setGrow(1.0f);

    this->searchButton = new brls::Button();
    this->searchButton->setStyle(&brls::BUTTONSTYLE_PRIMARY);
    this->searchButton->setText("Buscar mangá");
    this->searchButton->setDimensions(brls::View::AUTO, 60.0f);
    this->searchButton->setMargins(16.0f, 16.0f, 8.0f, 16.0f);
    this->searchButton->registerClickAction([this](brls::View*) {
        this->openKeyboard();
        return true;
    });
    this->addView(this->searchButton);

    // Everything below the search button lives in its own grow=1 band,
    // fully separate from searchButton, so the ScrollingFrame is never a
    // direct sibling of it in the column.
    brls::Box* contentArea = new brls::Box(brls::Axis::COLUMN);
    contentArea->setGrow(1.0f);
    contentArea->setWidthPercentage(100.0f);

    this->statusLabel = new brls::Label();
    this->statusLabel->setText("Toque em \"Buscar mangá\" para pesquisar.");
    this->statusLabel->setFontSize(18.0f);
    this->statusLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    this->statusLabel->setMargins(8.0f, 16.0f, 8.0f, 16.0f);
    contentArea->addView(this->statusLabel);

    this->scrollingFrame = new brls::ScrollingFrame();
    this->scrollingFrame->setGrow(1.0f);
    this->scrollingFrame->setWidthPercentage(100.0f);
    contentArea->addView(this->scrollingFrame);

    this->resultsContainer = new brls::Box(brls::Axis::COLUMN);
    this->resultsContainer->setWidthPercentage(100.0f);
    this->resultsContainer->setPadding(0.0f, 16.0f, 16.0f, 16.0f);

    // scrollingFrame->setContentView() is deferred to willAppear() - see
    // BrowseTab's constructor comment for why it can't run here yet.

    this->addView(contentArea);
}

SearchTab::~SearchTab()
{
    alive->store(false);
}

void SearchTab::willAppear(bool resetState)
{
    if (!this->contentViewAttached)
    {
        this->scrollingFrame->setContentView(this->resultsContainer);
        this->contentViewAttached = true;

        this->scrollSync = std::make_unique<ScrollContentSync>(this->scrollingFrame, this->resultsContainer);
        this->scrollSync->start();
    }

    Box::willAppear(resetState);
}

brls::View* SearchTab::create()
{
    return new SearchTab();
}

void SearchTab::openKeyboard()
{
    brls::Swkbd::openForText([this](std::string text) {
        if (!text.empty())
            this->performSearch(text, true);
    },
        "Buscar mangá", "Digite o título do mangá", 64, this->currentQuery);
}

void SearchTab::performSearch(const std::string& query, bool reset)
{
    if (reset)
    {
        this->currentQuery = query;
        this->offset = 0;
        this->knownTotal = 0;
        this->clearResults();
        this->setStatus("Buscando \"" + query + "\"...");
    }

    // Just hide it here, don't removeView() it: performSearch(false) is
    // called from *this exact button's own* click action (see below), and
    // removeView() deletes the view immediately - deleting a view from
    // inside a callback that's still executing as one of its own registered
    // actions is a use-after-free once Application::handleAction()'s loop
    // resumes afterward. The actual removal happens in appendResults()
    // instead, once the response comes back on a later main-thread tick,
    // safely outside that call stack.
    if (this->loadMoreButton)
    {
        // It's always focused at this exact point (you just clicked it),
        // and setVisibility(GONE) doesn't redirect focus away on its own
        // the way removeView()'s deletion does - leaving Application::
        // currentFocus stuck pointing at a now-hidden, effectively
        // unreachable view, which breaks all further navigation.
        if (brls::Application::getCurrentFocus() == this->loadMoreButton)
        {
            // Focus the last loaded card (right before loadMoreButton),
            // not searchButton at the very top of the screen - giving focus
            // to a view scrolls it into view, and jumping focus all the way
            // back up reset the scroll position to the top of the whole
            // list, forcing a full re-scroll past everything already seen.
            std::vector<brls::View*> children = this->resultsContainer->getChildren();
            brls::View* fallback = this->searchButton;
            if (children.size() >= 2)
                fallback = children[children.size() - 2];
            brls::Application::giveFocus(fallback);
        }

        this->loadMoreButton->setVisibility(brls::Visibility::GONE);
    }

    util::AliveFlag aliveCopy = this->alive;
    std::string searchQuery = this->currentQuery;
    int requestOffset = this->offset;

    util::spawnWorkerThread([this, aliveCopy, searchQuery, requestOffset]() {
        storage::Settings settings = storage::LibraryStore::LoadSettings();
        api::SearchResult result = api::MangaDexClient::SearchManga(searchQuery, requestOffset, kPageSize, settings.contentRatings, settings.preferredLanguage);

        util::runOnMainThread([this, aliveCopy, result]() {
            if (aliveCopy->load())
                this->appendResults(result);
        });
    });
}

void SearchTab::appendResults(const api::SearchResult& result)
{
    // Safe to actually delete it here (unlike in performSearch()): this
    // runs on a later, separate main-thread tick, never nested inside the
    // button's own click action call stack.
    if (this->loadMoreButton)
    {
        this->resultsContainer->removeView(this->loadMoreButton);
        this->loadMoreButton = nullptr;
    }

    this->knownTotal = result.total;
    this->offset += static_cast<int>(result.items.size());

    if (!result.ok && this->resultsContainer->getChildren().empty())
    {
        std::string detail = result.errorDetail.empty() ? ("HTTP " + std::to_string(result.httpStatus)) : result.errorDetail;
        this->setStatus("Erro de conexão (" + detail + ").\nVerifique sua internet e tente de novo.");
        return;
    }

    if (result.items.empty() && this->resultsContainer->getChildren().empty())
    {
        this->setStatus("Nenhum resultado encontrado.");
        return;
    }

    this->setStatus("");

    for (const auto& manga : result.items)
    {
        api::Manga mangaCopy = manga;
        MangaCard* card = new MangaCard(manga, [mangaCopy]() {
            brls::Application::pushActivity(new MangaDetailActivity(mangaCopy));
        });
        this->resultsContainer->addView(card);
    }

    if (this->offset < this->knownTotal)
    {
        this->loadMoreButton = new brls::Button();
        this->loadMoreButton->setText("Carregar mais");
        this->loadMoreButton->setDimensions(brls::View::AUTO, 56.0f);
        this->loadMoreButton->setMargins(16.0f, 0.0f, 16.0f, 0.0f);
        this->loadMoreButton->registerClickAction([this](brls::View*) {
            this->performSearch(this->currentQuery, false);
            return true;
        });
        this->resultsContainer->addView(this->loadMoreButton);
    }
}

void SearchTab::clearResults()
{
    // Only steal focus if it's actually about to be destroyed - see
    // BrowseTab::clearResults() (browse_tab.cpp) for why, and why
    // redirecting unconditionally is itself a bug (it breaks sidebar
    // navigation by yanking focus onto searchButton on every reload).
    if (isFocusInside(this->resultsContainer))
        brls::Application::giveFocus(this->searchButton);

    std::vector<brls::View*> children = this->resultsContainer->getChildren();
    for (brls::View* child : children)
        this->resultsContainer->removeView(child);
    this->loadMoreButton = nullptr;
}

void SearchTab::setStatus(const std::string& text)
{
    this->statusLabel->setText(text);
    this->statusLabel->setVisibility(text.empty() ? brls::Visibility::GONE : brls::Visibility::VISIBLE);
}

} // namespace ui
