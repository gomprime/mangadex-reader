#include "ui/library_tab.hpp"

#include <algorithm>

#include "storage/library_store.hpp"
#include "ui/manga_card.hpp"
#include "ui/manga_detail_activity.hpp"
#include "ui/reader_activity.hpp"

namespace ui
{

LibraryTab::LibraryTab()
    : brls::Box(brls::Axis::COLUMN)
{
    this->setGrow(1.0f);

    this->statusLabel = new brls::Label();
    this->statusLabel->setFontSize(18.0f);
    this->statusLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    this->statusLabel->setMargins(16.0f, 16.0f, 8.0f, 16.0f);
    this->statusLabel->setVisibility(brls::Visibility::GONE);
    this->addView(this->statusLabel);

    this->scrollingFrame = new brls::ScrollingFrame();
    this->scrollingFrame->setGrow(1.0f);
    this->scrollingFrame->setWidthPercentage(100.0f);
    this->addView(this->scrollingFrame);

    this->listContainer = new brls::Box(brls::Axis::COLUMN);
    this->listContainer->setWidthPercentage(100.0f);
    this->listContainer->setPadding(16.0f, 16.0f, 16.0f, 16.0f);

    // scrollingFrame->setContentView() is deferred to willAppear() - see
    // BrowseTab's constructor comment (browse_tab.cpp) for why it can't run
    // here yet.

    this->reload();
}

LibraryTab::~LibraryTab()
{
    alive->store(false);
}

void LibraryTab::willAppear(bool resetState)
{
    if (!this->contentViewAttached)
    {
        this->scrollingFrame->setContentView(this->listContainer);
        this->contentViewAttached = true;

        this->scrollSync = std::make_unique<ScrollContentSync>(this->scrollingFrame, this->listContainer);
        this->scrollSync->start();
    }

    Box::willAppear(resetState);
}

brls::View* LibraryTab::create()
{
    return new LibraryTab();
}

void LibraryTab::reload()
{
    std::vector<storage::LibraryEntry> entries = storage::LibraryStore::LoadLibrary();

    std::sort(entries.begin(), entries.end(), [](const storage::LibraryEntry& a, const storage::LibraryEntry& b) {
        return a.updatedAt > b.updatedAt;
    });

    if (entries.empty())
    {
        this->statusLabel->setText("Sua biblioteca está vazia.\nAdicione mangás pela busca ou novidades.");
        this->statusLabel->setVisibility(brls::Visibility::VISIBLE);
        return;
    }

    this->statusLabel->setVisibility(brls::Visibility::GONE);

    for (const auto& entry : entries)
    {
        api::Manga manga;
        manga.id = entry.mangaId;
        manga.title = entry.title;
        manga.coverFileName = entry.coverFileName;
        manga.status = entry.lastReadChapterId.empty()
            ? "Na biblioteca"
            : ("Continuar: Cap. " + entry.lastReadChapterNumber);

        storage::LibraryEntry entryCopy = entry;
        MangaCard* card = new MangaCard(manga, [manga, entryCopy]() {
            if (!entryCopy.lastReadChapterId.empty())
                brls::Application::pushActivity(new ReaderActivity(manga, entryCopy.lastReadChapterId, entryCopy.lastReadPage));
            else
                brls::Application::pushActivity(new MangaDetailActivity(manga));
        });

        this->listContainer->addView(card);
    }
}

} // namespace ui
