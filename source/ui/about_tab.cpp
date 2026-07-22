#include "ui/about_tab.hpp"

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
    this->addView(makeLine("Versão 1.0.0", false));
    this->addView(makeLine("Por gomprime", true));

    this->addView(makeHeader("Agradecimentos"));
    this->addView(makeLine("CostelaBR", false));
    this->addView(makeLine("AurelioEB", true));

    brls::Label* footer = new brls::Label();
    footer->setText("Leitor de mangás não-oficial usando a API pública do MangaDex.");
    footer->setFontSize(14.0f);
    footer->setHorizontalAlign(brls::HorizontalAlign::LEFT);
    footer->setMarginTop(8.0f);
    this->addView(footer);
}

brls::View* AboutTab::create()
{
    return new AboutTab();
}

} // namespace ui
