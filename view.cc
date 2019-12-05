// Copyright

#include "view.h"

#include <ostream>

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

//R->S->D

SkPoint View::ConvertPointFromChild(const View* child, SkPoint pt) const {
  if (child->Parent() != this) {
    if (!child->Parent()) {
      fprintf(stderr, "Child has no parent!\n");
      return SkPoint();
    }
    pt = child->Parent()->ConvertPointFromChild(child, pt);
    return ConvertPointFromChild(child->Parent(), pt);
  }
  // fprintf(stderr, "ConvPFC %f %f %f (in %f %f)\n", child->scale_,
  //         child->origin_.x(), child->origin_.y(), pt.x(), pt.y());
  pt *= child->scale_;
  pt.offset(child->origin_.x(), child->origin_.y());
  // fprintf(stderr, "ConvFPC (out %f %f)\n", pt.x(), pt.y());
  return pt;
}

SkSize View::ConvertSizeFromChild(const View* child, SkSize size) const {
  if (child->Parent() != this) {
    if (!child->Parent()) {
      fprintf(stderr, "Child has no parent!\n");
      return SkSize();
    }
    size = child->Parent()->ConvertSizeFromChild(child, size);
    return ConvertSizeFromChild(child->Parent(), size);
  }
  // fprintf(stderr, "CZFC %f %f * %f\n",
  //         size.width(), size.height(), child->scale_);
  size.fWidth *= child->scale_;
  size.fHeight *= child->scale_;
  // fprintf(stderr, "CZFC out %f %f\n",
  //         size.width(), size.height());
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
  if (rect.isEmpty())
    return;
  if (!parent_) {
    fprintf(stderr, "no parent!\n");
    return;
  }
  // fprintf(stderr, "SetNeedsDisplayInRect(%f %f %f %f) in progress (%f %f)\n",
  //         rect.left(), rect.top(), rect.right(), rect.bottom(),
  //         parent_->scale_, scale_);
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

View* View::MouseDown(MouseInputEvent ev) {
  for (ssize_t i = children_.size() - 1; i >= 0; i--) {
    View* child = children_[i];
    if (!child->Frame().contains(ev.position().x(), ev.position().y()))
      continue;
    MouseInputEvent child_evt = ev;
    child_evt.UpdateToChild(child, this);
    View* ret = child->MouseDown(child_evt);
    if (ret)
      return ret;
  }
  return nullptr;
}

void View::OnClick(MouseInputEvent ev) {
  for (ssize_t i = children_.size() - 1; i >= 0; i--) {
    View* child = children_[i];
    if (!child->Frame().contains(ev.position().x(), ev.position().y()))
      continue;
    MouseInputEvent child_evt = ev;
    child_evt.UpdateToChild(child, this);
    child->OnClick(child_evt);
    return;
  }
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

std::ostream& operator<<(std::ostream& os, const SkRect& rect) {
  return os << "{L: " << rect.fLeft << ", T: " << rect.fTop
     << ", R: " << rect.fRight << ", B: " << rect.fBottom << "}";
}
