#pragma once

#include <functional>

#include <borealis.hpp>

#include "api/models.hpp"
#include "util/lifetime.hpp"

namespace ui
{

// Cover + title cell used in search results, browse grids and the library tab.
// Downloads (or reuses the cached copy of) the manga cover in the background.
class MangaCard : public brls::Box
{
  public:
    MangaCard(const api::Manga& manga, std::function<void()> onClick);
    ~MangaCard() override;

  private:
    void loadCover(const std::string& mangaId, const std::string& coverFileName);

    brls::Image* cover;
    brls::Label* titleLabel;

    util::AliveFlag alive = util::makeAliveFlag();
};

} // namespace ui
