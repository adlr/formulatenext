// Copyright...

#include "rootview.h"

#include <emscripten.h>

#include "SkBitmap.h"

namespace formulate {

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
      PushCanvasXYWH($0, $1, $2, $3, $4);
    }, bitmap.getPixels(), irect.left(), irect.top(),
    bitmap.width(), bitmap.height());
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
