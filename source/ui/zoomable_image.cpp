#include "ui/zoomable_image.hpp"

namespace ui
{

void ZoomableImage::setZoom(float scale, float panX, float panY)
{
    this->zoomScale = scale;
    this->zoomPanX = panX;
    this->zoomPanY = panY;
}

void ZoomableImage::draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx)
{
    if (this->zoomScale == 1.0f)
    {
        brls::Image::draw(vg, x, y, width, height, style, ctx);
        return;
    }

    nvgSave(vg);

    float centerX = x + width / 2.0f;
    float centerY = y + height / 2.0f;

    nvgTranslate(vg, centerX, centerY);
    nvgScale(vg, this->zoomScale, this->zoomScale);
    nvgTranslate(vg, -centerX + this->zoomPanX, -centerY + this->zoomPanY);

    brls::Image::draw(vg, x, y, width, height, style, ctx);

    nvgRestore(vg);
}

} // namespace ui
