#include "ui/main_activity.hpp"

#include "ui/about_tab.hpp"
#include "ui/browse_tab.hpp"
#include "ui/library_tab.hpp"
#include "ui/search_tab.hpp"
#include "ui/settings_tab.hpp"

using namespace brls::literals;

namespace ui
{

brls::View* MainActivity::createContentView()
{
    brls::TabFrame* tabFrame = new brls::TabFrame();
    tabFrame->setTitle("MangaDex Reader");

    tabFrame->addTab("tabs/search"_i18n, &SearchTab::create);
    tabFrame->addTab("tabs/latest"_i18n, &BrowseTab::create);
    tabFrame->addTab("tabs/library"_i18n, &LibraryTab::create);
    tabFrame->addTab("tabs/settings"_i18n, &SettingsTab::create);
    tabFrame->addTab("tabs/about"_i18n, &AboutTab::create);

    return tabFrame;
}

} // namespace ui
