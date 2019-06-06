// Copyright

#include "scrollview.h"

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
    if (vertically_center_)
      neworigin.fY = (size_.height() - child->Height()) / 2;
    else
      neworigin.fY = 0;
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

}  // namespace formulate
