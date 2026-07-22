#pragma once

#include <memory>
#include <string>
#include <vector>

#include <borealis.hpp>

#include "api/models.hpp"
#include "ui/zoomable_image.hpp"
#include "util/lifetime.hpp"

namespace ui
{

// Full-screen paginated reader. Fetches the chapter list for the manga once
// (so ZL/ZR chapter switching works even when opened directly from the
// Library tab, which only knows the single last-read chapter id), then
// resolves each chapter's page URLs on demand via /at-home/server.
class ReaderActivity : public brls::Activity
{
  public:
    ReaderActivity(const api::Manga& manga, const std::string& startChapterId, int startPage);
    ~ReaderActivity() override;

    brls::View* createContentView() override;
    void onContentAvailable() override;

  private:
    void loadChapterList();
    void openChapterByIndex(int index);
    void loadCurrentChapterPages();
    void showPage(int pageIndex);
    void preloadAhead(int fromPage);
    void goToNextPage();
    void goToPrevPage();
    void goToNextChapter();
    void goToPrevChapter();
    void saveProgress();
    void goBack();
    void setStatus(const std::string& text);
    void updateHeaderLabel();

    void toggleZoom();
    void resetZoom();
    void applyZoomTransform();
    // Polled every ~16ms by zoomTicker while zoomed in - reads the left
    // stick directly via libnx (Borealis's action system is press-based,
    // not suited for continuous analog panning). stickX/stickY are
    // normalized to [-1, 1].
    void pollZoomPan(float stickX, float stickY);

    api::Manga manga;
    std::string startChapterId;
    int startPage;

    std::vector<api::Chapter> chapters; // ascending order, single language
    int currentChapterIndex = -1;
    std::vector<std::string> currentPageUrls;
    int currentPage = 0;

    // -1: start of chapter, -2: resolved to last page once pages are known
    int pendingStartPage = -1;

    ZoomableImage* pageImage = nullptr;
    brls::Label* headerLabel = nullptr;
    brls::Label* pageCounterLabel = nullptr;
    brls::Label* statusLabel = nullptr;
    brls::Label* zoomHintLabel = nullptr;

    static constexpr float kZoomedScale = 2.5f;
    bool zoomed = false;
    float panX = 0.0f; // pixels, relative to centered position
    float panY = 0.0f;

    class ZoomPanTicker; // defined in reader_activity.cpp - owns a raw PadState
    std::unique_ptr<ZoomPanTicker> zoomTicker;

    util::AliveFlag alive = util::makeAliveFlag();
};

} // namespace ui
