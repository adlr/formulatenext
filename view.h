// Copyright

#ifndef FORMULATE_VIEW_H__
#define FORMULATE_VIEW_H__

#include "SkCanvas.h"

namespace formulate {

class View {
 public:
  View() {}
  virtual ~View() {}
  virtual float Width() const = 0;
  virtual float Height() const = 0;
  virtual void Draw(SkCanvas* canvas, SkRect rect) = 0;
  virtual void MouseDown(SkPoint pt) {}
  virtual void MouseDrag(SkPoint pt) {}
  virtual void MouseUp(SkPoint pt) {}
};

template<typename T>
T Clamp(T cur, T min, T max) {
  if (cur < min) return min;
  if (cur > max) return max;
  return cur;
}

SkRect ScaleRect(SkRect rect, float scale);

}  // namespace formulate

#endif  // FORMULATE_VIEW_H__
