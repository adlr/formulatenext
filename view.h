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
  virtual void Draw(SkCanvas* canvas) = 0;
};

template<typename T>
T Clamp(T cur, T min, T max) {
  if (cur < min) return min;
  if (cur > max) return max;
  return cur;
}

}  // namespace formulate

#endif  // FORMULATE_VIEW_H__
