#include "ui/main_activity.hpp"

#include "ui/about_tab.hpp"
#include "ui/browse_tab.hpp"
#include "ui/library_tab.hpp"
#include "ui/search_tab.hpp"
#include "ui/settings_tab.hpp"

namespace ui
{

brls::View* MainActivity::createContentView()
{
    brls::TabFrame* tabFrame = new brls::TabFrame();
    tabFrame->setTitle("MangaDex Reader");

    tabFrame->addTab("Buscar", &SearchTab::create);
    tabFrame->addTab("Novidades", &BrowseTab::create);
    tabFrame->addTab("Biblioteca", &LibraryTab::create);
    tabFrame->addTab("Configurações", &SettingsTab::create);
    tabFrame->addTab("Sobre", &AboutTab::create);

    return tabFrame;
}

} // namespace ui
