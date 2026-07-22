#include "ui/manga_detail_activity.hpp"

#include "api/mangadex_client.hpp"
#include "storage/cache_manager.hpp"
#include "storage/library_store.hpp"
#include "ui/reader_activity.hpp"
#include "util/main_thread.hpp"
#include "util/worker_thread.hpp"

namespace ui
{

namespace
{
    std::string joinTags(const std::vector<std::string>& tags)
    {
        std::string out;
        for (size_t i = 0; i < tags.size(); i++)
        {
            if (i > 0)
                out += "  •  ";
            out += tags[i];
        }
        return out;
    }
}

MangaDetailActivity::MangaDetailActivity(const api::Manga& manga)
    : manga(manga)
{
}

MangaDetailActivity::~MangaDetailActivity()
{
    alive->store(false);
}

brls::View* MangaDetailActivity::createContentView()
{
    this->scrollingFrame = new brls::ScrollingFrame();
    this->scrollingFrame->setWidthPercentage(100.0f);
    this->scrollingFrame->setHeightPercentage(100.0f);

    brls::Box* root = new brls::Box(brls::Axis::COLUMN);
    root->setWidthPercentage(100.0f);
    root->setPadding(24.0f, 32.0f, 24.0f, 32.0f);

    brls::Box* headerRow = new brls::Box(brls::Axis::ROW);
    headerRow->setWidthPercentage(100.0f);

    this->coverView = new brls::Image();
    this->coverView->setDimensions(180.0f, 252.0f);
    this->coverView->setScalingType(brls::ImageScalingType::CROP);
    this->coverView->setMarginRight(24.0f);
    headerRow->addView(this->coverView);

    brls::Box* infoColumn = new brls::Box(brls::Axis::COLUMN);
    infoColumn->setGrow(1.0f);

    this->titleLabel = new brls::Label();
    this->titleLabel->setText(this->manga.title);
    this->titleLabel->setFontSize(28.0f);
    this->titleLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    infoColumn->addView(this->titleLabel);

    this->tagsLabel = new brls::Label();
    this->tagsLabel->setText(joinTags(this->manga.tags));
    this->tagsLabel->setFontSize(14.0f);
    this->tagsLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    this->tagsLabel->setMarginTop(8.0f);
    infoColumn->addView(this->tagsLabel);

    brls::Box* buttonRow = new brls::Box(brls::Axis::ROW);
    buttonRow->setMarginTop(16.0f);

    this->continueButton = new brls::Button();
    this->continueButton->setStyle(&brls::BUTTONSTYLE_PRIMARY);
    this->continueButton->setText("Ler do início");
    this->continueButton->setDimensions(220.0f, 56.0f);
    this->continueButton->setMarginRight(12.0f);
    this->continueButton->registerClickAction([this](brls::View*) {
        std::vector<storage::LibraryEntry> library = storage::LibraryStore::LoadLibrary();
        for (const auto& entry : library)
        {
            if (entry.mangaId == this->manga.id && !entry.lastReadChapterId.empty())
            {
                this->openChapter(api::Chapter { entry.lastReadChapterId, entry.lastReadChapterNumber, "", "", "", 0, "" }, entry.lastReadPage);
                return true;
            }
        }
        brls::Application::notify("Escolha um capítulo na lista abaixo.");
        return true;
    });
    buttonRow->addView(this->continueButton);

    this->favoriteButton = new brls::Button();
    this->favoriteButton->setStyle(&brls::BUTTONSTYLE_BORDERED);
    this->favoriteButton->setText("+ Biblioteca");
    this->favoriteButton->setDimensions(200.0f, 56.0f);
    this->favoriteButton->registerClickAction([this](brls::View*) {
        if (storage::LibraryStore::Contains(this->manga.id))
        {
            storage::LibraryStore::Remove(this->manga.id);
        }
        else
        {
            storage::LibraryEntry entry;
            entry.mangaId = this->manga.id;
            entry.title = this->manga.title;
            entry.coverFileName = this->manga.coverFileName;
            storage::LibraryStore::Upsert(entry);
        }
        this->refreshContinueReadingButton();
        return true;
    });
    buttonRow->addView(this->favoriteButton);

    infoColumn->addView(buttonRow);

    headerRow->addView(infoColumn);
    root->addView(headerRow);

    this->descriptionLabel = new brls::Label();
    this->descriptionLabel->setText(this->manga.description);
    this->descriptionLabel->setFontSize(16.0f);
    this->descriptionLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    this->descriptionLabel->setMarginTop(20.0f);
    root->addView(this->descriptionLabel);

    brls::Label* chaptersHeader = new brls::Label();
    chaptersHeader->setText("Capítulos");
    chaptersHeader->setFontSize(22.0f);
    chaptersHeader->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    chaptersHeader->setMarginTop(24.0f);
    chaptersHeader->setMarginBottom(8.0f);
    root->addView(chaptersHeader);

    this->chaptersContainer = new brls::Box(brls::Axis::COLUMN);
    this->chaptersContainer->setWidthPercentage(100.0f);
    root->addView(this->chaptersContainer);

    this->scrollingFrame->setContentView(root);
    return this->scrollingFrame;
}

void MangaDetailActivity::onContentAvailable()
{
    this->registerAction("Voltar", brls::BUTTON_B, [](brls::View*) {
        brls::Application::popActivity();
        return true;
    });

    this->refreshContinueReadingButton();
    this->loadDetail();
    this->loadChapters();
}

void MangaDetailActivity::loadDetail()
{
    util::AliveFlag aliveCopy = this->alive;
    std::string mangaId = this->manga.id;

    util::spawnWorkerThread([this, aliveCopy, mangaId]() {
        api::Manga detail;
        bool ok = api::MangaDexClient::GetMangaDetail(mangaId, detail);
        if (!ok)
            return;

        util::runOnMainThread([this, aliveCopy, detail]() {
            if (!aliveCopy->load())
                return;

            this->manga = detail;
            this->titleLabel->setText(detail.title);
            this->descriptionLabel->setText(detail.description);
            this->tagsLabel->setText(joinTags(detail.tags));
            this->scrollingFrame->invalidate();

            if (!detail.coverFileName.empty())
            {
                brls::Image* coverView = this->coverView;
                std::string coverMangaId = detail.id;
                std::string coverFileName = detail.coverFileName;
                util::AliveFlag innerAlive = aliveCopy;

                util::spawnWorkerThread([coverView, coverMangaId, coverFileName, innerAlive]() {
                    std::string url = api::MangaDexClient::CoverUrl(coverMangaId, coverFileName, "512");
                    std::string path = storage::CacheManager::GetOrDownloadCover(coverMangaId, url);
                    if (path.empty())
                        return;

                    util::runOnMainThread([coverView, path, innerAlive]() {
                        if (innerAlive->load())
                            coverView->setImageFromFile(path);
                    });
                });
            }
        });
    });
}

void MangaDetailActivity::loadChapters()
{
    util::AliveFlag aliveCopy = this->alive;
    std::string mangaId = this->manga.id;
    int offset = this->chapterOffset;

    util::spawnWorkerThread([this, aliveCopy, mangaId, offset]() {
        storage::Settings settings = storage::LibraryStore::LoadSettings();
        api::ChapterFeed feed = api::MangaDexClient::GetChapterFeed(mangaId, settings.preferredLanguage, offset, kChapterPageSize);

        util::runOnMainThread([this, aliveCopy, feed]() {
            if (aliveCopy->load())
                this->appendChapters(feed);
        });
    });
}

void MangaDetailActivity::appendChapters(const api::ChapterFeed& feed)
{
    const std::vector<api::Chapter>& newChapters = feed.items;
    this->chapterOffset += static_cast<int>(newChapters.size());

    if (newChapters.empty() && this->chaptersContainer->getChildren().empty())
    {
        brls::Label* emptyLabel = new brls::Label();
        // feed.ok distinguishes a genuinely empty result (no chapters in
        // this language) from a failed request (network/rate limit/TLS)
        // silently showing the same "nothing here" message - which was
        // very misleading, since it looked identical to a real "not
        // translated" case and gave no indication a retry might help.
        if (!feed.ok)
        {
            std::string detail = feed.errorDetail.empty() ? ("HTTP " + std::to_string(feed.httpStatus)) : feed.errorDetail;
            emptyLabel->setText("Erro de conexão (" + detail + ").\nVerifique sua internet e tente de novo.");
        }
        else
        {
            emptyLabel->setText("Nenhum capítulo disponível no idioma configurado.");
        }
        emptyLabel->setFontSize(16.0f);
        emptyLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        this->chaptersContainer->addView(emptyLabel);
        this->scrollingFrame->invalidate();
        return;
    }

    for (const auto& chapter : newChapters)
    {
        bool isExternal = !chapter.externalUrl.empty();

        brls::Box* row = new brls::Box(brls::Axis::ROW);
        row->setWidthPercentage(100.0f);
        row->setHeight(56.0f);
        row->setPadding(8.0f, 12.0f, 8.0f, 12.0f);
        row->setMarginBottom(4.0f);
        row->setAlignItems(brls::AlignItems::CENTER);
        row->setCornerRadius(6.0f);

        std::string label = chapter.volume.empty() ? "" : ("Vol. " + chapter.volume + " ");
        label += chapter.chapterNumber.empty() ? "Extra" : ("Cap. " + chapter.chapterNumber);
        if (!chapter.title.empty())
            label += " - " + chapter.title;
        if (isExternal)
            label += " (somente no site oficial)";

        brls::Label* rowLabel = new brls::Label();
        rowLabel->setText(label);
        rowLabel->setFontSize(18.0f);
        rowLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        rowLabel->setGrow(1.0f);
        row->addView(rowLabel);

        if (isExternal)
        {
            // Not hosted on MangaDex at all (a licensed/official release
            // elsewhere, e.g. MangaPlus) - /at-home/server has no page data
            // to serve for it, so opening it in our reader would just fail
            // with a confusing "download error". Keep the row visible (so
            // the chapter list isn't mysteriously missing entries) but
            // dimmed and non-interactive instead.
            rowLabel->setTextColor(nvgRGB(140, 140, 140));
        }
        else
        {
            row->setFocusable(true);

            api::Chapter chapterCopy = chapter;
            row->registerClickAction([this, chapterCopy](brls::View*) {
                this->openChapter(chapterCopy, 0);
                return true;
            });
        }

        this->chaptersContainer->addView(row);
    }

    // chaptersContainer is a nested child of the ScrollingFrame's content
    // view, not the content view itself - adding rows here doesn't
    // automatically refresh the frame's cached scroll bounds, leaving the
    // whole screen blank until some other event (e.g. the back transition)
    // forces a fresh layout. Force it here instead.
    this->scrollingFrame->invalidate();
}

void MangaDetailActivity::openChapter(const api::Chapter& chapter, int startPage)
{
    brls::Application::pushActivity(new ReaderActivity(this->manga, chapter.id, startPage));
}

void MangaDetailActivity::refreshContinueReadingButton()
{
    std::vector<storage::LibraryEntry> library = storage::LibraryStore::LoadLibrary();
    bool inLibrary = false;
    bool hasProgress = false;
    std::string chapterLabel;

    for (const auto& entry : library)
    {
        if (entry.mangaId == this->manga.id)
        {
            inLibrary = true;
            if (!entry.lastReadChapterId.empty())
            {
                hasProgress = true;
                chapterLabel = entry.lastReadChapterNumber;
            }
            break;
        }
    }

    // Plain ASCII, not a Unicode checkmark: the loaded font doesn't have
    // that glyph and it rendered as a missing-glyph box (same issue as the
    // Material Icons codepoints tried earlier for the settings arrows).
    this->favoriteButton->setText(inLibrary ? "- Biblioteca" : "+ Biblioteca");
    this->continueButton->setText(hasProgress ? ("Continuar (Cap. " + chapterLabel + ")") : "Escolha um capítulo");
}

} // namespace ui
