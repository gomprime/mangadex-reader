#include "ui/about_tab.hpp"

#include "util/main_thread.hpp"
#include "util/worker_thread.hpp"

using namespace brls::literals;

namespace ui
{

namespace
{
    brls::Label* makeHeader(const std::string& text)
    {
        brls::Label* label = new brls::Label();
        label->setText(text);
        label->setFontSize(22.0f);
        label->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        return label;
    }

    // A literal "\n" inside a single Label's text does not render as a line
    // break here - nvgTextBox() doesn't interpret it as one, and the font
    // has no visible glyph for the raw control character, so it shows up
    // as a broken/missing-glyph box instead. Use one Label per line, same
    // as everywhere else in this app.
    brls::Label* makeLine(const std::string& text, bool lastLine)
    {
        brls::Label* label = new brls::Label();
        label->setText(text);
        label->setFontSize(18.0f);
        label->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        label->setMarginTop(4.0f);
        if (lastLine)
            label->setMarginBottom(24.0f);
        return label;
    }
}

AboutTab::AboutTab()
    : brls::Box(brls::Axis::COLUMN)
{
    this->setGrow(1.0f);
    this->setPadding(24.0f, 32.0f, 24.0f, 32.0f);

    this->addView(makeHeader("MangaDex Reader"));
    this->addView(makeLine("about/version"_i18n, false));
    this->addView(makeLine("about/by"_i18n, true));

    this->addView(makeHeader("about/thanks_header"_i18n));
    this->addView(makeLine("CostelaBR", false));
    this->addView(makeLine("AurelioEB", true));

    brls::Label* footer = new brls::Label();
    footer->setText("about/footer"_i18n);
    footer->setFontSize(14.0f);
    footer->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    footer->setMarginTop(8.0f);
    this->addView(footer);

    this->addView(makeHeader("about/update_header"_i18n));

    this->updateStatusLabel = new brls::Label();
    this->updateStatusLabel->setText("about/checking_update"_i18n);
    this->updateStatusLabel->setFontSize(16.0f);
    this->updateStatusLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    this->updateStatusLabel->setMarginTop(8.0f);
    this->addView(this->updateStatusLabel);

    this->updateButton = new brls::Button();
    this->updateButton->setStyle(&brls::BUTTONSTYLE_PRIMARY);
    this->updateButton->setText("about/download_button"_i18n);
    this->updateButton->setDimensions(240.0f, 56.0f);
    this->updateButton->setMarginTop(12.0f);
    this->updateButton->setVisibility(brls::Visibility::GONE);
    this->updateButton->registerClickAction([this](brls::View*) {
        this->downloadUpdate();
        return true;
    });
    this->addView(this->updateButton);

    this->checkForUpdate();
}

AboutTab::~AboutTab()
{
    alive->store(false);
}

brls::View* AboutTab::create()
{
    return new AboutTab();
}

void AboutTab::checkForUpdate()
{
    util::AliveFlag aliveCopy = this->alive;

    util::spawnWorkerThread([this, aliveCopy]() {
        api::UpdateInfo info = api::UpdateClient::CheckLatestRelease();

        util::runOnMainThread([this, aliveCopy, info]() {
            if (aliveCopy->load())
                this->onUpdateChecked(info);
        });
    });
}

void AboutTab::onUpdateChecked(const api::UpdateInfo& info)
{
    if (!info.ok)
    {
        this->updateStatusLabel->setText(brls::getStr("about/update_check_failed", info.errorDetail));
        return;
    }

    if (info.downloadUrl.empty() || !api::UpdateClient::IsNewerVersion(APP_VERSION_STRING, info.latestVersion))
    {
        this->updateStatusLabel->setText("about/up_to_date"_i18n);
        return;
    }

    this->pendingDownloadUrl = info.downloadUrl;
    this->updateStatusLabel->setText(brls::getStr("about/update_available", info.latestVersion));
    this->updateButton->setVisibility(brls::Visibility::VISIBLE);
}

void AboutTab::downloadUpdate()
{
    if (this->pendingDownloadUrl.empty())
        return;

    this->updateButton->setVisibility(brls::Visibility::GONE);
    this->updateStatusLabel->setText("about/downloading"_i18n);

    util::AliveFlag aliveCopy = this->alive;
    std::string url = this->pendingDownloadUrl;

    util::spawnWorkerThread([this, aliveCopy, url]() {
        std::string error;
        bool ok = api::UpdateClient::DownloadAndInstall(url, error);

        util::runOnMainThread([this, aliveCopy, ok, error]() {
            if (!aliveCopy->load())
                return;

            if (ok)
                this->updateStatusLabel->setText("about/update_installed"_i18n);
            else
                this->updateStatusLabel->setText(brls::getStr("about/update_failed", error));
        });
    });
}

} // namespace ui
