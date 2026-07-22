#pragma once

#include <borealis.hpp>

#include "api/models.hpp"
#include "util/lifetime.hpp"

namespace ui
{

// Manga details screen: cover, synopsis, tags and the chapter list.
// Only manga.id is guaranteed to be populated by the caller (list screens only
// have summary data); the full detail (description, tags) is fetched on appear.
class MangaDetailActivity : public brls::Activity
{
  public:
    explicit MangaDetailActivity(const api::Manga& manga);
    ~MangaDetailActivity() override;

    brls::View* createContentView() override;
    void onContentAvailable() override;

  private:
    void loadDetail();
    void loadChapters();
    void appendChapters(const api::ChapterFeed& feed);
    void openChapter(const api::Chapter& chapter, int startPage);
    void refreshContinueReadingButton();

    api::Manga manga;

    brls::ScrollingFrame* scrollingFrame = nullptr;
    brls::Image* coverView = nullptr;
    brls::Label* titleLabel = nullptr;
    brls::Label* descriptionLabel = nullptr;
    brls::Label* tagsLabel = nullptr;
    brls::Box* chaptersContainer = nullptr;
    brls::Button* continueButton = nullptr;
    brls::Button* favoriteButton = nullptr;

    int chapterOffset = 0;
    static constexpr int kChapterPageSize = 100;

    util::AliveFlag alive = util::makeAliveFlag();
};

} // namespace ui
