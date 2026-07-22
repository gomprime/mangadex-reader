#include "ui/reader_activity.hpp"

#include <cmath>

#include <switch.h>

#include "api/mangadex_client.hpp"
#include "storage/cache_manager.hpp"
#include "storage/library_store.hpp"
#include "util/main_thread.hpp"
#include "util/worker_thread.hpp"

namespace ui
{

namespace
{
    constexpr int kPreloadAheadCount = 2;
}

// Reads the left analog stick directly via libnx and feeds it to
// ReaderActivity::pollZoomPan() every ~16ms while zoomed in. Borealis owns
// its own PadState for button actions, but its action system is press-based
// and not suited for continuous analog panning, so this ticker keeps a
// second, independent PadState just for the stick axis.
class ReaderActivity::ZoomPanTicker : public brls::RepeatingTask
{
  public:
    ZoomPanTicker(ReaderActivity* owner)
        : RepeatingTask(16)
        , owner(owner)
    {
        padInitializeDefault(&this->pad);
    }

    void run() override
    {
        padUpdate(&this->pad);
        HidAnalogStickState stick = padGetStickPos(&this->pad, 0);
        this->owner->pollZoomPan(static_cast<float>(stick.x) / JOYSTICK_MAX, static_cast<float>(stick.y) / JOYSTICK_MAX);
    }

  private:
    ReaderActivity* owner;
    PadState pad;
};

ReaderActivity::ReaderActivity(const api::Manga& manga, const std::string& startChapterId, int startPage)
    : manga(manga)
    , startChapterId(startChapterId)
    , startPage(startPage)
{
}

ReaderActivity::~ReaderActivity()
{
    alive->store(false);
}

brls::View* ReaderActivity::createContentView()
{
    brls::Box* root = new brls::Box(brls::Axis::COLUMN);
    root->setWidthPercentage(100.0f);
    root->setHeightPercentage(100.0f);
    root->setBackgroundColor(nvgRGB(0, 0, 0));

    brls::Box* headerRow = new brls::Box(brls::Axis::ROW);
    headerRow->setWidthPercentage(100.0f);
    headerRow->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
    headerRow->setPadding(8.0f, 16.0f, 8.0f, 16.0f);

    this->headerLabel = new brls::Label();
    this->headerLabel->setFontSize(18.0f);
    this->headerLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    this->headerLabel->setGrow(1.0f);
    this->headerLabel->setTextColor(nvgRGB(255, 255, 255));
    headerRow->addView(this->headerLabel);

    this->pageCounterLabel = new brls::Label();
    this->pageCounterLabel->setFontSize(18.0f);
    this->pageCounterLabel->setHorizontalAlign(brls::HorizontalAlign::RIGHT);
    this->pageCounterLabel->setTextColor(nvgRGB(255, 255, 255));
    headerRow->addView(this->pageCounterLabel);

    root->addView(headerRow);

    brls::Box* pageArea = new brls::Box(brls::Axis::COLUMN);
    pageArea->setWidthPercentage(100.0f);
    pageArea->setGrow(1.0f);
    pageArea->setJustifyContent(brls::JustifyContent::CENTER);
    pageArea->setAlignItems(brls::AlignItems::CENTER);

    this->pageImage = new ZoomableImage();
    this->pageImage->setGrow(1.0f);
    this->pageImage->setWidthPercentage(100.0f);
    this->pageImage->setHeightPercentage(100.0f);
    this->pageImage->setScalingType(brls::ImageScalingType::FIT);
    pageArea->addView(this->pageImage);

    this->statusLabel = new brls::Label();
    this->statusLabel->setFontSize(22.0f);
    this->statusLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    this->statusLabel->setText("Carregando...");
    this->statusLabel->setTextColor(nvgRGB(255, 255, 255));
    pageArea->addView(this->statusLabel);

    root->addView(pageArea);

    brls::Label* hintLabel = new brls::Label();
    hintLabel->setText("D-pad/analógico: página   ZL/ZR: capítulo   Y: zoom   B: voltar");
    hintLabel->setFontSize(16.0f);
    hintLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    hintLabel->setMargins(4.0f, 8.0f, 8.0f, 8.0f);
    hintLabel->setTextColor(nvgRGB(255, 255, 255));
    root->addView(hintLabel);

    this->zoomHintLabel = new brls::Label();
    this->zoomHintLabel->setText("Zoom ativo: analógico esquerdo move a página (D-pad não troca de página)");
    this->zoomHintLabel->setFontSize(16.0f);
    this->zoomHintLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    this->zoomHintLabel->setMargins(0.0f, 8.0f, 8.0f, 8.0f);
    this->zoomHintLabel->setTextColor(nvgRGB(255, 220, 100));
    this->zoomHintLabel->setVisibility(brls::Visibility::GONE);
    root->addView(this->zoomHintLabel);

    return root;
}

void ReaderActivity::onContentAvailable()
{
    // BUTTON_LEFT/RIGHT fire for the D-pad *and* for the left stick tilted
    // sideways (libnx's HidNpadButton_AnyLeft/AnyRight, which Borealis maps
    // both to - see switch_input.cpp). That stick is also used for zoom pan,
    // so while zoomed these must be ignored or panning would keep flipping
    // pages; BUTTON_LB/BUTTON_RB (physical shoulder buttons only) still work.
    this->registerAction("Página anterior", brls::BUTTON_LEFT, [this](brls::View*) {
        if (this->zoomed)
            return false;
        this->goToPrevPage();
        return true;
    });
    this->registerAction("Próxima página", brls::BUTTON_RIGHT, [this](brls::View*) {
        if (this->zoomed)
            return false;
        this->goToNextPage();
        return true;
    });
    this->registerAction("Página anterior", brls::BUTTON_LB, [this](brls::View*) { this->goToPrevPage(); return true; });
    this->registerAction("Próxima página", brls::BUTTON_RB, [this](brls::View*) { this->goToNextPage(); return true; });
    this->registerAction("Capítulo anterior", brls::BUTTON_LT, [this](brls::View*) { this->goToPrevChapter(); return true; });
    this->registerAction("Próximo capítulo", brls::BUTTON_RT, [this](brls::View*) { this->goToNextChapter(); return true; });
    this->registerAction("Voltar", brls::BUTTON_B, [this](brls::View*) { this->goBack(); return true; });
    this->registerAction("Zoom", brls::BUTTON_Y, [this](brls::View*) { this->toggleZoom(); return true; });

    this->loadChapterList();
}

void ReaderActivity::toggleZoom()
{
    this->zoomed = !this->zoomed;

    if (this->zoomed)
    {
        if (!this->zoomTicker)
            this->zoomTicker = std::make_unique<ZoomPanTicker>(this);
        this->zoomTicker->start();
    }
    else
    {
        if (this->zoomTicker)
            this->zoomTicker->stop();
        this->panX = 0.0f;
        this->panY = 0.0f;
    }

    this->applyZoomTransform();
}

void ReaderActivity::resetZoom()
{
    if (this->zoomTicker)
        this->zoomTicker->stop();

    this->zoomed = false;
    this->panX  = 0.0f;
    this->panY  = 0.0f;

    this->applyZoomTransform();
}

void ReaderActivity::applyZoomTransform()
{
    float scale = this->zoomed ? kZoomedScale : 1.0f;
    this->pageImage->setZoom(scale, this->panX, this->panY);

    if (this->zoomHintLabel)
        this->zoomHintLabel->setVisibility(this->zoomed ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
}

void ReaderActivity::pollZoomPan(float stickX, float stickY)
{
    if (!this->zoomed)
        return;

    constexpr float kDeadzone = 0.15f;
    constexpr float kPanSpeed = 12.0f; // pre-scale pixels per tick at full deflection

    if (std::fabs(stickX) < kDeadzone)
        stickX = 0.0f;
    if (std::fabs(stickY) < kDeadzone)
        stickY = 0.0f;

    if (stickX == 0.0f && stickY == 0.0f)
        return;

    float width  = this->pageImage->getWidth();
    float height = this->pageImage->getHeight();

    // Content this->pageImage draws is scaled by kZoomedScale around its
    // center; this is how far it can be panned (pre-scale units) before an
    // edge would pull inward past the box and expose empty space.
    float maxPanX = (kZoomedScale - 1.0f) * width / (2.0f * kZoomedScale);
    float maxPanY = (kZoomedScale - 1.0f) * height / (2.0f * kZoomedScale);

    // Stick right/up "looks" toward that side of the page, so the page
    // content itself moves the opposite way on screen.
    this->panX -= stickX * kPanSpeed;
    this->panY += stickY * kPanSpeed;

    if (this->panX > maxPanX)
        this->panX = maxPanX;
    if (this->panX < -maxPanX)
        this->panX = -maxPanX;
    if (this->panY > maxPanY)
        this->panY = maxPanY;
    if (this->panY < -maxPanY)
        this->panY = -maxPanY;

    this->applyZoomTransform();
}

void ReaderActivity::loadChapterList()
{
    this->setStatus("Carregando capítulos...");

    util::AliveFlag aliveCopy = this->alive;
    std::string mangaId = this->manga.id;
    std::string targetChapterId = this->startChapterId;

    util::spawnWorkerThread([this, aliveCopy, mangaId, targetChapterId]() {
        storage::Settings settings = storage::LibraryStore::LoadSettings();
        api::ChapterFeed feed = api::MangaDexClient::GetChapterFeed(mangaId, settings.preferredLanguage, 0, 100);

        util::runOnMainThread([this, aliveCopy, feed, targetChapterId]() {
            if (!aliveCopy->load())
                return;

            if (feed.items.empty())
            {
                this->setStatus("Não foi possível carregar os capítulos.\nVerifique sua conexão.");
                return;
            }

            this->chapters = feed.items;

            int index = 0;
            for (size_t i = 0; i < this->chapters.size(); i++)
            {
                if (this->chapters[i].id == targetChapterId)
                {
                    index = static_cast<int>(i);
                    break;
                }
            }

            this->openChapterByIndex(index);
        });
    });
}

void ReaderActivity::openChapterByIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(this->chapters.size()))
        return;

    this->currentChapterIndex = index;
    this->currentPageUrls.clear();
    this->updateHeaderLabel();
    this->loadCurrentChapterPages();
}

void ReaderActivity::loadCurrentChapterPages()
{
    this->setStatus("Carregando página...");

    util::AliveFlag aliveCopy = this->alive;
    std::string chapterId = this->chapters[this->currentChapterIndex].id;

    util::spawnWorkerThread([this, aliveCopy, chapterId]() {
        storage::Settings settings = storage::LibraryStore::LoadSettings();
        api::ImageQuality quality = (settings.imageQuality == "data") ? api::ImageQuality::Data : api::ImageQuality::DataSaver;
        api::AtHomeServer result = api::MangaDexClient::GetAtHomeServer(chapterId, quality);

        util::runOnMainThread([this, aliveCopy, result]() {
            if (!aliveCopy->load())
                return;

            if (!result.ok)
            {
                this->setStatus("Falha ao carregar as páginas.\nVerifique sua conexão.");
                return;
            }

            this->currentPageUrls = result.pageUrls;

            int target = 0;
            if (this->pendingStartPage == -2)
                target = static_cast<int>(this->currentPageUrls.size()) - 1;
            else if (this->currentChapterIndex >= 0 && this->pendingStartPage == -1 && !this->startChapterId.empty()
                && this->chapters[this->currentChapterIndex].id == this->startChapterId)
                target = this->startPage;

            this->pendingStartPage = -1;
            this->showPage(target < 0 ? 0 : target);
        });
    });
}

void ReaderActivity::showPage(int pageIndex)
{
    if (pageIndex < 0 || pageIndex >= static_cast<int>(this->currentPageUrls.size()))
        return;

    this->currentPage = pageIndex;
    this->pageCounterLabel->setText(std::to_string(pageIndex + 1) + " / " + std::to_string(this->currentPageUrls.size()));
    this->setStatus("Carregando página...");
    this->resetZoom();

    util::AliveFlag aliveCopy = this->alive;
    std::string chapterId = this->chapters[this->currentChapterIndex].id;
    std::string pageUrl = this->currentPageUrls[pageIndex];
    ZoomableImage* imageView = this->pageImage;

    util::spawnWorkerThread([this, aliveCopy, chapterId, pageIndex, pageUrl, imageView]() {
        std::string path = storage::CacheManager::GetOrDownloadPage(chapterId, pageIndex, pageUrl);

        util::runOnMainThread([this, aliveCopy, path, imageView]() {
            if (!aliveCopy->load())
                return;

            if (path.empty())
            {
                this->setStatus("Falha ao baixar a página.");
                return;
            }

            imageView->setImageFromFile(path);
            this->setStatus("");
        });
    },
        /* priority */ true); // jump ahead of any stale preload backlog from paging quickly

    this->saveProgress();
    this->preloadAhead(pageIndex + 1);
}

void ReaderActivity::preloadAhead(int fromPage)
{
    util::AliveFlag aliveCopy = this->alive;
    std::string chapterId = this->chapters[this->currentChapterIndex].id;

    for (int i = fromPage; i < fromPage + kPreloadAheadCount && i < static_cast<int>(this->currentPageUrls.size()); i++)
    {
        std::string pageUrl = this->currentPageUrls[i];
        util::spawnWorkerThread([aliveCopy, chapterId, i, pageUrl]() {
            if (aliveCopy->load())
                storage::CacheManager::GetOrDownloadPage(chapterId, i, pageUrl);
        });
    }
}

void ReaderActivity::goToNextPage()
{
    if (this->currentChapterIndex < 0)
        return;

    if (this->currentPage + 1 < static_cast<int>(this->currentPageUrls.size()))
        this->showPage(this->currentPage + 1);
    else
        this->goToNextChapter();
}

void ReaderActivity::goToPrevPage()
{
    if (this->currentChapterIndex < 0)
        return;

    if (this->currentPage > 0)
        this->showPage(this->currentPage - 1);
    else
        this->goToPrevChapter();
}

void ReaderActivity::goToNextChapter()
{
    if (this->currentChapterIndex + 1 < static_cast<int>(this->chapters.size()))
    {
        this->pendingStartPage = -1;
        this->openChapterByIndex(this->currentChapterIndex + 1);
    }
    else
    {
        brls::Application::notify("Este é o último capítulo disponível.");
    }
}

void ReaderActivity::goToPrevChapter()
{
    if (this->currentChapterIndex - 1 >= 0)
    {
        this->pendingStartPage = -2;
        this->openChapterByIndex(this->currentChapterIndex - 1);
    }
    else
    {
        brls::Application::notify("Este é o primeiro capítulo disponível.");
    }
}

void ReaderActivity::saveProgress()
{
    if (this->currentChapterIndex < 0)
        return;

    storage::LibraryEntry entry;
    entry.mangaId = this->manga.id;
    entry.title = this->manga.title;
    entry.coverFileName = this->manga.coverFileName;
    entry.lastReadChapterId = this->chapters[this->currentChapterIndex].id;
    entry.lastReadChapterNumber = this->chapters[this->currentChapterIndex].chapterNumber;
    entry.lastReadPage = this->currentPage;

    storage::LibraryStore::Upsert(entry);
}

void ReaderActivity::goBack()
{
    this->saveProgress();
    brls::Application::popActivity();
}

void ReaderActivity::setStatus(const std::string& text)
{
    this->statusLabel->setText(text);
    this->statusLabel->setVisibility(text.empty() ? brls::Visibility::GONE : brls::Visibility::VISIBLE);
}

void ReaderActivity::updateHeaderLabel()
{
    if (this->currentChapterIndex < 0)
        return;

    const api::Chapter& chapter = this->chapters[this->currentChapterIndex];
    std::string text = this->manga.title;
    if (!chapter.chapterNumber.empty())
        text += " - Cap. " + chapter.chapterNumber;
    if (!chapter.title.empty())
        text += ": " + chapter.title;

    this->headerLabel->setText(text);
}

} // namespace ui
