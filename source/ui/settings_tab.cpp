#include "ui/settings_tab.hpp"

#include <algorithm>

#include "storage/cache_manager.hpp"
#include "util/main_thread.hpp"
#include "util/worker_thread.hpp"

namespace ui
{

namespace
{
    constexpr const char* kRatingIds[4] = { "safe", "suggestive", "erotica", "pornographic" };
    constexpr const char* kRatingLabels[4] = { "Seguro", "Sugestivo", "Erótica", "Pornográfico" };

    // MangaDex translatedLanguage/availableTranslatedLanguage codes. Empty
    // code = no language filter at all (search/browse show every language,
    // reader chapter list falls back to "en" - see mangadex_client.cpp).
    struct LanguageOption
    {
        const char* code;
        const char* label;
    };

    constexpr LanguageOption kLanguages[] = {
        { "", "Qualquer idioma" },
        { "pt-br", "Português (Brasil)" },
        { "en", "English" },
        { "es", "Español" },
        { "es-la", "Español (LatAm)" },
        { "fr", "Français" },
        { "de", "Deutsch" },
        { "it", "Italiano" },
        { "ru", "Русский" },
        { "ja", "日本語" },
        { "ko", "한국어" },
        { "zh", "中文" },
    };
    constexpr int kLanguageCount = sizeof(kLanguages) / sizeof(kLanguages[0]);

    // Plain ASCII instead of Material Icons codepoints: the icon font isn't
    // guaranteed to be loaded (Application logs a warning and skips the
    // fallback registration if it fails), and testing showed the codepoints
    // rendering as nothing. These glyphs are guaranteed present in any font.
    constexpr const char* kIconArrowUp = "^";
    constexpr const char* kIconArrowDown = "v";
    constexpr const char* kIconArrowLeft = "<";
    constexpr const char* kIconArrowRight = ">";

    int findLanguageIndex(const std::string& code)
    {
        for (int i = 0; i < kLanguageCount; i++)
        {
            if (code == kLanguages[i].code)
                return i;
        }
        return 0;
    }
}

SettingsTab::SettingsTab()
    : brls::Box(brls::Axis::COLUMN)
{
    this->settings = storage::LibraryStore::LoadSettings();

    this->setGrow(1.0f);
    this->setPadding(24.0f, 32.0f, 24.0f, 32.0f);

    brls::Label* ratingsHeader = new brls::Label();
    ratingsHeader->setText("Classificação de conteúdo");
    ratingsHeader->setFontSize(22.0f);
    ratingsHeader->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    this->addView(ratingsHeader);

    brls::Box* ratingsRow = new brls::Box(brls::Axis::ROW);
    ratingsRow->setMarginTop(12.0f);
    ratingsRow->setMarginBottom(24.0f);

    for (int i = 0; i < 4; i++)
    {
        brls::Button* button = new brls::Button();
        button->setText(kRatingLabels[i]);
        button->setDimensions(180.0f, 56.0f);
        button->setMarginRight(12.0f);

        std::string rating = kRatingIds[i];
        button->registerClickAction([this, rating, button](brls::View*) {
            this->toggleContentRating(rating, button);
            return true;
        });

        this->ratingButtons[i] = button;
        ratingsRow->addView(button);
    }
    this->addView(ratingsRow);

    brls::Label* qualityHeader = new brls::Label();
    qualityHeader->setText("Qualidade das imagens");
    qualityHeader->setFontSize(22.0f);
    qualityHeader->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    this->addView(qualityHeader);

    brls::Box* qualityRow = new brls::Box(brls::Axis::ROW);
    qualityRow->setMarginTop(12.0f);
    qualityRow->setMarginBottom(24.0f);

    this->dataSaverButton = new brls::Button();
    this->dataSaverButton->setText("Economia de dados");
    this->dataSaverButton->setDimensions(240.0f, 56.0f);
    this->dataSaverButton->setMarginRight(12.0f);
    this->dataSaverButton->registerClickAction([this](brls::View*) {
        this->setImageQuality("data-saver");
        return true;
    });
    qualityRow->addView(this->dataSaverButton);

    this->dataButton = new brls::Button();
    this->dataButton->setText("Qualidade original");
    this->dataButton->setDimensions(240.0f, 56.0f);
    this->dataButton->registerClickAction([this](brls::View*) {
        this->setImageQuality("data");
        return true;
    });
    qualityRow->addView(this->dataButton);

    this->addView(qualityRow);

    brls::Label* languageHeader = new brls::Label();
    languageHeader->setText("Idioma de busca e leitura");
    languageHeader->setFontSize(22.0f);
    languageHeader->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    this->addView(languageHeader);

    brls::Box* languageRow = new brls::Box(brls::Axis::ROW);
    languageRow->setMarginTop(12.0f);
    languageRow->setMarginBottom(24.0f);
    languageRow->setAlignItems(brls::AlignItems::CENTER);

    brls::Button* prevLanguageButton = new brls::Button();
    prevLanguageButton->setText(kIconArrowLeft);
    prevLanguageButton->setDimensions(56.0f, 56.0f);
    prevLanguageButton->setMarginRight(12.0f);
    prevLanguageButton->registerClickAction([this](brls::View*) {
        this->cycleLanguage(-1);
        return true;
    });
    languageRow->addView(prevLanguageButton);

    this->languageLabel = new brls::Label();
    this->languageLabel->setFontSize(18.0f);
    this->languageLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    this->languageLabel->setDimensions(260.0f, brls::View::AUTO);
    languageRow->addView(this->languageLabel);

    brls::Button* nextLanguageButton = new brls::Button();
    nextLanguageButton->setText(kIconArrowRight);
    nextLanguageButton->setDimensions(56.0f, 56.0f);
    nextLanguageButton->setMarginLeft(12.0f);
    nextLanguageButton->registerClickAction([this](brls::View*) {
        this->cycleLanguage(1);
        return true;
    });
    languageRow->addView(nextLanguageButton);

    this->addView(languageRow);

    brls::Label* cacheHeader = new brls::Label();
    cacheHeader->setText("Limite de cache no cartão SD");
    cacheHeader->setFontSize(22.0f);
    cacheHeader->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    this->addView(cacheHeader);

    brls::Box* cacheRow = new brls::Box(brls::Axis::ROW);
    cacheRow->setMarginTop(12.0f);
    cacheRow->setAlignItems(brls::AlignItems::CENTER);

    brls::Button* decreaseButton = new brls::Button();
    decreaseButton->setText(kIconArrowDown);
    decreaseButton->setDimensions(56.0f, 56.0f);
    decreaseButton->setMarginRight(12.0f);
    decreaseButton->registerClickAction([this](brls::View*) {
        this->adjustCacheLimit(-100);
        return true;
    });
    cacheRow->addView(decreaseButton);

    this->cacheLimitLabel = new brls::Label();
    this->cacheLimitLabel->setFontSize(18.0f);
    this->cacheLimitLabel->setMarginRight(12.0f);
    cacheRow->addView(this->cacheLimitLabel);

    brls::Button* increaseButton = new brls::Button();
    increaseButton->setText(kIconArrowUp);
    increaseButton->setDimensions(56.0f, 56.0f);
    increaseButton->setMarginRight(24.0f);
    increaseButton->registerClickAction([this](brls::View*) {
        this->adjustCacheLimit(100);
        return true;
    });
    cacheRow->addView(increaseButton);

    brls::Button* clearButton = new brls::Button();
    clearButton->setStyle(&brls::BUTTONSTYLE_BORDERED);
    clearButton->setText("Limpar cache agora");
    clearButton->setDimensions(220.0f, 56.0f);
    clearButton->registerClickAction([this](brls::View*) {
        this->statusLabel->setText("Limpando cache...");
        this->statusLabel->setVisibility(brls::Visibility::VISIBLE);

        util::AliveFlag aliveCopy = this->alive;
        util::spawnWorkerThread([this, aliveCopy]() {
            storage::CacheManager::ClearAll();
            util::runOnMainThread([this, aliveCopy]() {
                if (aliveCopy->load())
                {
                    this->statusLabel->setText("Cache limpo.");
                }
            });
        });
        return true;
    });
    cacheRow->addView(clearButton);

    this->addView(cacheRow);

    this->statusLabel = new brls::Label();
    this->statusLabel->setFontSize(16.0f);
    this->statusLabel->setMarginTop(16.0f);
    this->statusLabel->setVisibility(brls::Visibility::GONE);
    this->addView(this->statusLabel);

    this->refreshLabels();
}

SettingsTab::~SettingsTab()
{
    alive->store(false);
}

brls::View* SettingsTab::create()
{
    return new SettingsTab();
}

void SettingsTab::toggleContentRating(const std::string& rating, brls::Button*)
{
    auto& ratings = this->settings.contentRatings;
    auto it = std::find(ratings.begin(), ratings.end(), rating);

    if (it != ratings.end())
    {
        // Always keep at least one rating enabled.
        if (ratings.size() > 1)
            ratings.erase(it);
    }
    else
    {
        ratings.push_back(rating);
    }

    this->save();
    this->refreshLabels();
}

void SettingsTab::setImageQuality(const std::string& quality)
{
    this->settings.imageQuality = quality;
    this->save();
    this->refreshLabels();
}

void SettingsTab::adjustCacheLimit(int deltaMB)
{
    this->settings.cacheLimitMB = std::max(100, this->settings.cacheLimitMB + deltaMB);
    this->save();
    this->refreshLabels();
}

void SettingsTab::cycleLanguage(int direction)
{
    int index = findLanguageIndex(this->settings.preferredLanguage);
    index = (index + direction + kLanguageCount) % kLanguageCount;
    this->settings.preferredLanguage = kLanguages[index].code;

    this->save();
    this->refreshLabels();
}

void SettingsTab::refreshLabels()
{
    for (int i = 0; i < 4; i++)
    {
        bool enabled = std::find(this->settings.contentRatings.begin(), this->settings.contentRatings.end(),
                           kRatingIds[i])
            != this->settings.contentRatings.end();
        this->ratingButtons[i]->setStyle(enabled ? &brls::BUTTONSTYLE_PRIMARY : &brls::BUTTONSTYLE_DEFAULT);
    }

    bool dataSaver = this->settings.imageQuality != "data";
    this->dataSaverButton->setStyle(dataSaver ? &brls::BUTTONSTYLE_PRIMARY : &brls::BUTTONSTYLE_DEFAULT);
    this->dataButton->setStyle(dataSaver ? &brls::BUTTONSTYLE_DEFAULT : &brls::BUTTONSTYLE_PRIMARY);

    this->cacheLimitLabel->setText(std::to_string(this->settings.cacheLimitMB) + " MB");

    this->languageLabel->setText(kLanguages[findLanguageIndex(this->settings.preferredLanguage)].label);
}

void SettingsTab::save()
{
    storage::LibraryStore::SaveSettings(this->settings);
}

} // namespace ui
