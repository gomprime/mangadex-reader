#pragma once

#include <borealis.hpp>

namespace ui
{

// brls::Image with a zoom/pan transform applied purely at draw time (not
// part of the yoga layout), so panning while zoomed doesn't trigger a
// relayout every frame - it's just a cheap nanovg transform around the
// normal draw call.
class ZoomableImage : public brls::Image
{
  public:
    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override;

    // panX/panY are in the image's local (pre-scale) coordinate space.
    void setZoom(float scale, float panX, float panY);

  private:
    float zoomScale = 1.0f;
    float zoomPanX = 0.0f;
    float zoomPanY = 0.0f;
};

} // namespace ui
