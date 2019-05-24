// Copyright

#include "view.h"

namespace formulate {

void View::SetSize(SkSize size) {
  size_ = size;
}

SkPoint View::ConvertPointToChild(const View* child, SkPoint pt) const {
  if (child->Parent() != this) {
    if (!child->Parent()) {
      fprintf(stderr, "Child has no parent!\n");
      return SkPoint();
    }
    pt = ConvertPointToChild(child->Parent(), pt);
    return child->Parent()->ConvertPointToChild(child, pt);
  }
  pt.offset(-child->origin_.x(), -child->origin_.y());
  pt *= 1.0 / child->scale_;
  return pt;
}

SkSize View::ConvertSizeToChild(const View* child, SkSize size) const {
  if (child->Parent() != this) {
    if (!child->Parent()) {
      fprintf(stderr, "Child has no parent!\n");
      return SkSize();
    }
    size = ConvertSizeToChild(child->Parent(), size);
    return child->Parent()->ConvertSizeToChild(child, size);
  }
  size.fWidth /= child->scale_;
  size.fHeight /= child->scale_;
  return size;
}

SkPoint View::ConvertPointFromChild(const View* child, SkPoint pt) const {
  if (child->Parent() != this) {
    if (!child->Parent()) {
      fprintf(stderr, "Child has no parent!\n");
      return SkPoint();
    }
    pt = child->Parent()->ConvertPointFromChild(child, pt);
    return ConvertPointToChild(child->Parent(), pt);
  }
  pt *= child->scale_;
  pt.offset(child->origin_.x(), child->origin_.y());
  return pt;
}

SkSize View::ConvertSizeFromChild(const View* child, SkSize size) const {
  if (child->Parent() != this) {
    if (!child->Parent()) {
      fprintf(stderr, "Child has no parent!\n");
      return SkSize();
    }
    size = child->Parent()->ConvertSizeToChild(child, size);
    return ConvertSizeToChild(child->Parent(), size);
  }
  size.fWidth *= child->scale_;
  size.fHeight *= child->scale_;
  return size;
}

void View::Draw(SkCanvas* canvas, SkRect rect) {
  // Default implementation: draw subviews
  for (size_t i = 0; i < children_.size(); i++) {
    View* child = children_[i];
    canvas->save();
    canvas->translate(child->origin_.x(), child->origin_.y());
    canvas->scale(child->scale_, child->scale_);
    SkRect childdraw = child->Bounds();
    childdraw.intersect(ConvertRectToChild(child, rect));
    child->Draw(canvas, childdraw);
    canvas->restore();
  }
}

void View::SetNeedsDisplayInRect(const SkRect& rect) {
  if (!parent_) {
    fprintf(stderr, "no parent!\n");
    return;
  }
  parent_->SetNeedsDisplayInRect(parent_->ConvertRectFromChild(this, rect));
}

SkRect View::VisibleSubrect() const {
  const SkRect bounds = Bounds();
  if (!parent_)
    return bounds;
  SkRect parent_visible = parent_->ConvertRectToChild(this,
                                                      parent_->VisibleSubrect());
  parent_visible.intersect(bounds);
  return parent_visible;
}

void View::Dump(int indent) const {
  char spaces[indent + 1];
  for (size_t i = 0; i < indent; i++)
    spaces[i] = ' ';
  spaces[indent] = '\0';
  fprintf(stderr, "%sView: (%f, %f) (%f, %f) (%f)\n", spaces,
          origin_.x(), origin_.y(), size_.width(), size_.height(),
          scale_);
  for (size_t i = 0; i < children_.size(); i++) {
    children_[i]->Dump(indent + 2);
  }
}

SkRect ScaleRect(SkRect rect, float scale) {
  rect.fLeft *= scale;
  rect.fTop *= scale;
  rect.fRight *= scale;
  rect.fBottom *= scale;
  return rect;
}

}  // namespace formulate
