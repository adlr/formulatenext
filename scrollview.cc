// Copyright

#include "scrollview.h"

#include <emscripten.h>

#include "SkBitmap.h"

#include "docview.h"

namespace formulate {

void ScrollView::Draw(SkCanvas* canvas) {
  canvas->scale(scale_, scale_);
  canvas->translate(-origin_.x(), -origin_.y());
  child_->Draw(canvas);
}

void ScrollView::RepositionChild() {
  if (size_.width() > child_->Width()) {
    // center child
    origin_.fX = -(size_.width() - child_->Width()) / 2;
  } else {
    origin_.fX = Clamp(origin_.fX, 0.0f,
                       child_->Width() - size_.width());
  }
  if (size_.height() > child_->Height()) {
    // center child
    origin_.fY = -(size_.height() - child_->Height()) / 2;
  } else {
    origin_.fY = Clamp(origin_.fY, 0.0f,
                       child_->Height() - size_.height());
  }
  EM_ASM_({
      bridge_setSize($0, $1, $2, $3);
    }, child_->Width(), child_->Height(), origin_.fX, origin_.fY);
}

void ScrollView::DoDraw() {
  // Allocate a buffer
  SkISize bufsize({static_cast<int32_t>(Width()),
        static_cast<int32_t>(Height())});
  SkBitmap bitmap;
  bitmap.setInfo(SkImageInfo::Make(bufsize.width(),
                                   bufsize.height(),
                                   kRGBA_8888_SkColorType,
                                   kUnpremul_SkAlphaType));
  if (!bitmap.tryAllocPixels()) {
    fprintf(stderr, "failed to alloc bitmap\n");
    return;
  }
  SkCanvas offscreen(bitmap);
  Draw(&offscreen);
  // Push to HTML canvas now
  EM_ASM_({
      PushCanvas($0, $1, $2);
    }, bitmap.getPixels(), bitmap.width(), bitmap.height());
}

}  // namespace formulate
