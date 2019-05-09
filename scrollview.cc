// Copyright

#include "scrollview.h"

#include <emscripten.h>

#include "SkBitmap.h"

#include "docview.h"

namespace formulate {

void ScrollView::Draw(SkCanvas* canvas, SkRect rect) {
  canvas->scale(scale_, scale_);
  rect = ScaleRect(rect, scale_);
  canvas->translate(-origin_.x(), -origin_.y());
  rect.offset(origin_);
  child_->Draw(canvas, rect);
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

SkPoint ScrollView::ChildVisibleCenter() const {
  // Find center of this view
  SkPoint center = SkPoint::Make(size_.width() / 2,
                                 size_.height() / 2);
  // shift by origin of child
  center.offset(origin_.x(), origin_.y());
  return center;
}

void ScrollView::CenterOnChildPoint(SkPoint point) {
  // Find center of this view
  SkPoint center = SkPoint::Make(size_.width() / 2,
                                 size_.height() / 2);
  origin_ = SkPoint::Make(point.x() - center.x(),
                          point.y() - center.y());
  RepositionChild();
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
  Draw(&offscreen, SkRect::MakeWH(bitmap.width(), bitmap.height()));
  // Push to HTML canvas now
  EM_ASM_({
      PushCanvas($0, $1, $2);
    }, bitmap.getPixels(), bitmap.width(), bitmap.height());
}

void ScrollView::MouseDown(SkPoint pt) {
  pt.offset(origin_.x(), origin_.y());
  SkRect child_bounds =
    SkRect::MakeXYWH(0, 0, child_->Width(), child_->Height());
  if (child_bounds.contains(pt.x(), pt.y())) {
    sent_child_mousedown_ = true;
    child_->MouseDown(pt);
  }
}
void ScrollView::MouseDrag(SkPoint pt) {
  pt.offset(origin_.x(), origin_.y());
  if (sent_child_mousedown_)
    child_->MouseDrag(pt);
}
void ScrollView::MouseUp(SkPoint pt) {
  pt.offset(origin_.x(), origin_.y());
  if (sent_child_mousedown_) {
    child_->MouseUp(pt);
    sent_child_mousedown_ = false;
  }
}

}  // namespace formulate
