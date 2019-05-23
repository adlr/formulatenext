// Copyright

#ifndef FORMULATE_VIEW_H__
#define FORMULATE_VIEW_H__

#include <vector>

#include "SkCanvas.h"

namespace formulate {

class View {
 public:
  View() {}
  virtual ~View() {}
  View* Parent() const { return parent_; }
  void AddChild(View* child) {
    children_.push_back(child);
    child->parent_ = this;
  }

  float Width() const { return size_.fWidth; }
  float Height() const { return size_.fHeight; }
  SkRect Bounds() const { return SkRect::MakeSize(size_); }
  SkRect Frame() const {  // In parent's coordinate space
    return SkRect::MakeXYWH(origin_.x(), origin_.y(),
                            size_.width() * scale_,
                            size_.height() * scale_);
  }
  virtual void SetSize(SkSize size);
  virtual void SetScale(float scale) { scale_ = scale; }
  virtual void SetOrigin(SkPoint origin) { origin_ = origin; }
  SkPoint origin() const { return origin_; }

  // Child may be indirect:
  SkPoint ConvertPointToChild(const View* child, SkPoint pt) const;
  SkSize ConvertSizeToChild(const View* child, SkSize size) const;
  SkRect ConvertRectToChild(const View* child, SkRect rect) const {
    SkPoint origin = ConvertPointToChild(child,
                                         SkPoint::Make(rect.left(),
                                                       rect.top()));
    SkSize size = ConvertSizeToChild(child,
                                     SkSize::Make(rect.width(),
                                                  rect.height()));
    return SkRect::MakeXYWH(origin.x(), origin.y(),
                            size.width(), size.height());
  }
  SkPoint ConvertPointFromChild(const View* child, SkPoint pt) const;
  SkSize ConvertSizeFromChild(const View* child, SkSize size) const;
  SkRect ConvertRectFromChild(const View* child, SkRect rect) const {
    SkPoint origin = ConvertPointFromChild(child,
                                         SkPoint::Make(rect.left(),
                                                       rect.top()));
    SkSize size = ConvertSizeFromChild(child,
                                     SkSize::Make(rect.width(),
                                                  rect.height()));
    return SkRect::MakeXYWH(origin.x(), origin.y(),
                            size.width(), size.height());
  }

  virtual void Draw(SkCanvas* canvas, SkRect rect);
  virtual void SetNeedsDisplayInRect(const SkRect& rect);
  void SetNeedsDisplay() { SetNeedsDisplayInRect(Bounds()); }

  // Returns the visible subrectangle of Bounds(). May be larger
  // than what is actually visible.
  SkRect VisibleSubrect() const;

  virtual void MouseDown(SkPoint pt) {}
  virtual void MouseDrag(SkPoint pt) {}
  virtual void MouseUp(SkPoint pt) {}

  void Dump(int indent) const;
 protected:
  SkPoint origin_;  // in parent's coordinates
  SkSize size_;  // in this' coordinates
  float scale_{1.0f};  // relative to parent
  View* parent_{nullptr};
  std::vector<View*> children_;
};

template<typename T>
T Clamp(T cur, T min, T max) {
  if (min > max) return Clamp(cur, max, min);
  if (cur < min) return min;
  if (cur > max) return max;
  return cur;
}

SkRect ScaleRect(SkRect rect, float scale);

}  // namespace formulate

#endif  // FORMULATE_VIEW_H__
