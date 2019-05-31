// Copyright...

#include "rootview.h"

#include <emscripten.h>

#include "SkBitmap.h"

namespace formulate {

void MouseInputEvent::UpdateToChild(View* child, View* from_parent) {
  position_ = from_parent->ConvertPointToChild(child, position_);
}

void RootView::SetNeedsDisplayInRect(const SkRect& rect) {
  if (!redraw_) {
    fprintf(stderr, "Don't have redraw object!\n");
    return;
  }
  redraw_->AddRect(rect);
}

void RootView::SetRedraw(ScopedRedraw* redraw) {
  if (redraw_ && redraw) {
    fprintf(stderr, "Already have a redraw object!\n");
  } else if (!redraw_ && !redraw) {
    fprintf(stderr, "Redraw object already null\n");
  }
  redraw_ = redraw;
}

void RootView::DoDraw(SkRect rect) {
  if (!rect.intersect(Bounds())) {
    fprintf(stderr, "Nothing to draw\n");
    return;
  }
  // Get pixels to redraw
  SkIRect irect;
  rect.roundOut(&irect);
  // Allocate a buffer
  SkBitmap bitmap;
  bitmap.setInfo(SkImageInfo::Make(irect.width(), irect.height(),
                                   kRGBA_8888_SkColorType,
                                   kUnpremul_SkAlphaType));
  if (!bitmap.tryAllocPixels()) {
    fprintf(stderr, "failed to alloc bitmap\n");
    return;
  }
  SkCanvas offscreen(bitmap);
  offscreen.translate(-irect.left(), -irect.top());
  Draw(&offscreen, SkRect::Make(irect));
  // Push to HTML canvas now
  EM_ASM_({
      PushCanvasXYWH($0, $1, $2, $3, $4, $5);
    }, id_, bitmap.getPixels(), irect.left(), irect.top(),
    bitmap.width(), bitmap.height());
}

View* RootView::MouseDown(MouseInputEvent ev) {
  if (drag_consumer_) {
    fprintf(stderr, "Already have drag consumer!\n");
    return nullptr;
  }
  drag_consumer_ = View::MouseDown(ev);
  return drag_consumer_;
}

void RootView::MouseDrag(MouseInputEvent ev) {
  if (drag_consumer_) {
    ev.UpdateToChild(drag_consumer_, this);
    drag_consumer_->MouseDrag(ev);
  }
}

void RootView::MouseUp(MouseInputEvent ev) {
  if (drag_consumer_) {
    ev.UpdateToChild(drag_consumer_, this);
    drag_consumer_->MouseUp(ev);
    drag_consumer_ = nullptr;
  }
}

ScopedRedraw::~ScopedRedraw() {
  if (!rect_.isEmpty()) {
    view_->DoDraw(rect_);
  }
  view_->SetRedraw(nullptr);
  view_ = nullptr;
  rect_ = SkRect::MakeEmpty();
}

}  // namespace formulate
