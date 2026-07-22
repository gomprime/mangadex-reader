#pragma once

#include <borealis.hpp>

#include "storage/library_store.hpp"
#include "util/lifetime.hpp"

namespace ui
{

// "Configurações" tab: content rating filters, image quality, cache limit and
// a "limpar cache" action. Everything is saved to settings.json immediately.
class SettingsTab : public brls::Box
{
  public:
    SettingsTab();
    ~SettingsTab() override;

    static brls::View* create();

  private:
    void toggleContentRating(const std::string& rating, brls::Button* button);
    void setImageQuality(const std::string& quality);
    void adjustCacheLimit(int deltaMB);
    void cycleLanguage(int direction);
    void refreshLabels();
    void save();

    storage::Settings settings;

    brls::Button* ratingButtons[4];
    brls::Button* dataButton;
    brls::Button* dataSaverButton;
    brls::Label* cacheLimitLabel;
    brls::Label* languageLabel;
    brls::Label* statusLabel;

    util::AliveFlag alive = util::makeAliveFlag();
};

} // namespace ui
