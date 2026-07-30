#pragma once

#include <borealis.hpp>

#include "api/update_client.hpp"
#include "util/lifetime.hpp"

namespace ui
{

// "Sobre" tab: app name, version, author, acknowledgments, and a manual
// update check against this project's GitHub releases.
class AboutTab : public brls::Box
{
  public:
    AboutTab();
    ~AboutTab() override;

    static brls::View* create();

  private:
    void checkForUpdate();
    void onUpdateChecked(const api::UpdateInfo& info);
    void downloadUpdate();

    brls::Label* updateStatusLabel = nullptr;
    brls::Button* updateButton = nullptr;
    std::string pendingDownloadUrl;

    util::AliveFlag alive = util::makeAliveFlag();
};

} // namespace ui
