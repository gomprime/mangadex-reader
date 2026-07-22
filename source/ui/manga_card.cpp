#include "ui/manga_card.hpp"

#include "api/mangadex_client.hpp"
#include "storage/cache_manager.hpp"
#include "util/main_thread.hpp"
#include "util/worker_thread.hpp"

namespace ui
{

namespace
{
    constexpr float kCoverWidth = 70.0f;
    constexpr float kCoverHeight = 98.0f;
    constexpr float kRowHeight = 116.0f;
}

MangaCard::MangaCard(const api::Manga& manga, std::function<void()> onClick)
    : brls::Box(brls::Axis::ROW)
{
    this->setHeight(kRowHeight);
    this->setWidthPercentage(100.0f);
    this->setPadding(8.0f, 12.0f, 8.0f, 12.0f);
    this->setMarginBottom(6.0f);
    this->setAlignItems(brls::AlignItems::CENTER);
    this->setCornerRadius(6.0f);
    this->setFocusable(true);
    this->registerClickAction([onClick](brls::View*) {
        if (onClick)
            onClick();
        return true;
    });

    this->cover = new brls::Image();
    this->cover->setDimensions(kCoverWidth, kCoverHeight);
    this->cover->setScalingType(brls::ImageScalingType::CROP);
    this->cover->setMarginRight(16.0f);
    this->addView(this->cover);

    brls::Box* textColumn = new brls::Box(brls::Axis::COLUMN);
    textColumn->setGrow(1.0f);

    this->titleLabel = new brls::Label();
    this->titleLabel->setText(manga.title.empty() ? "(sem título)" : manga.title);
    this->titleLabel->setFontSize(20.0f);
    this->titleLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    textColumn->addView(this->titleLabel);

    if (!manga.status.empty())
    {
        brls::Label* statusLabel = new brls::Label();
        statusLabel->setText(manga.status);
        statusLabel->setFontSize(14.0f);
        statusLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        statusLabel->setMarginTop(4.0f);
        textColumn->addView(statusLabel);
    }

    this->addView(textColumn);

    this->loadCover(manga.id, manga.coverFileName);
}

MangaCard::~MangaCard()
{
    alive->store(false);
}

void MangaCard::loadCover(const std::string& mangaId, const std::string& coverFileName)
{
    if (coverFileName.empty())
        return;

    util::AliveFlag aliveCopy = this->alive;
    brls::Image* coverView = this->cover;

    util::spawnWorkerThread([mangaId, coverFileName, aliveCopy, coverView]() {
        std::string url = api::MangaDexClient::CoverUrl(mangaId, coverFileName);
        std::string path = storage::CacheManager::GetOrDownloadCover(mangaId, url);
        if (path.empty())
            return;

        util::runOnMainThread([aliveCopy, coverView, path]() {
            if (aliveCopy->load())
                coverView->setImageFromFile(path);
        });
    });
}

} // namespace ui
