// Copyright

#include "scrollview.h"

#include <emscripten.h>

#include "SkBitmap.h"

#include "docview.h"

namespace formulate {

void ScrollView::RepositionChild() {
  if (children_.size() != 1) {
    fprintf(stderr, "ScrollView has wrong number of children! (%zu)\n",
            children_.size());
    return;
  }
  View* child = children_[0];
  SkPoint neworigin = child->origin();

  if (size_.width() > child->Width()) {
    // center child
    neworigin.fX = (size_.width() - child->Width()) / 2;
  } else {
    neworigin.fX = Clamp(neworigin.fX, 0.0f,
                         size_.width() - child->Width());
  }
  if (size_.height() > child->Height()) {
    // center child
    neworigin.fY = (size_.height() - child->Height()) / 2;
  } else {
    neworigin.fY = Clamp(neworigin.fY, 0.0f,
                         size_.height() - child->Height());
  }
  child->SetOrigin(neworigin);
  bridge_setSize(this, child->Width(), child->Height(),
                 std::max(-child->origin().x(), 0.0f),
                 std::max(-child->origin().y(), 0.0f));
}

SkPoint ScrollView::ChildVisibleCenter() const {
  if (children_.size() != 1) {
    fprintf(stderr, "ScrollView has wrong number of children! (%zu)\n",
            children_.size());
    return SkPoint();
  }
  View* child = children_[0];
  // Find center of this view
  SkPoint center = SkPoint::Make(size_.width() / 2,
                                 size_.height() / 2);
  // shift by origin of child
  center.offset(-child->origin().x(), -child->origin().y());
  return center;
}

void ScrollView::CenterOnChildPoint(SkPoint point) {
  if (children_.size() != 1) {
    fprintf(stderr, "ScrollView has wrong number of children! (%zu)\n",
            children_.size());
    return;
  }
  View* child = children_[0];
  // Find center of this view
  SkPoint center = SkPoint::Make(size_.width() / 2,
                                 size_.height() / 2);
  child->SetOrigin(SkPoint::Make(center.x() - point.x(),
                                 center.y() - point.y()));
  RepositionChild();
}

// void ScrollView::DoDraw() {
//   // Allocate a buffer
//   SkISize bufsize({static_cast<int32_t>(Width()),
//         static_cast<int32_t>(Height())});
//   SkBitmap bitmap;
//   bitmap.setInfo(SkImageInfo::Make(bufsize.width(),
//                                    bufsize.height(),
//                                    kRGBA_8888_SkColorType,
//                                    kUnpremul_SkAlphaType));
//   if (!bitmap.tryAllocPixels()) {
//     fprintf(stderr, "failed to alloc bitmap\n");
//     return;
//   }
//   SkCanvas offscreen(bitmap);
//   Draw(&offscreen, SkRect::MakeWH(bitmap.width(), bitmap.height()));
//   // Push to HTML canvas now
//   EM_ASM_({
//       PushCanvas($0, $1, $2);
//     }, bitmap.getPixels(), bitmap.width(), bitmap.height());
// }

// void ScrollView::MouseDown(SkPoint pt) {
//   pt.offset(origin_.x(), origin_.y());
//   SkRect child_bounds =
//     SkRect::MakeXYWH(0, 0, child_->Width(), child_->Height());
//   if (child_bounds.contains(pt.x(), pt.y())) {
//     sent_child_mousedown_ = true;
//     child_->MouseDown(pt);
//   }
// }
// void ScrollView::MouseDrag(SkPoint pt) {
//   pt.offset(origin_.x(), origin_.y());
//   if (sent_child_mousedown_)
//     child_->MouseDrag(pt);
// }
// void ScrollView::MouseUp(SkPoint pt) {
//   pt.offset(origin_.x(), origin_.y());
//   if (sent_child_mousedown_) {
//     child_->MouseUp(pt);
//     sent_child_mousedown_ = false;
//   }
// }

}  // namespace formulate
