#pragma once

#include <borealis.hpp>

namespace ui
{

// Borealis's ScrollingFrame positions its content view through a one-shot
// "detached" absolute position snapshot taken when setContentView() runs,
// not a live-recomputed value like a normal view's getX()/getY(). If outer
// layout (tab attach, header sizing, sidebar width) keeps settling after
// that snapshot is taken, the content goes stale - and ScrollingFrame's own
// re-sync hook (onLayout) doesn't reliably fire again once things stabilize.
// Keep it correct by re-applying the position continuously instead of
// hunting for the one "right" moment to capture it once.
class ScrollContentSync : public brls::RepeatingTask
{
  public:
    ScrollContentSync(brls::ScrollingFrame* frame, brls::View* content)
        : RepeatingTask(100)
        , frame(frame)
        , content(content)
    {
    }

    void run() override
    {
        content->setDetachedPosition(frame->getX(), frame->getY());
        content->setMaxWidth(frame->getWidth());
    }

  private:
    brls::ScrollingFrame* frame;
    brls::View* content;
};

} // namespace ui
