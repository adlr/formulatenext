// Copyright

#include "SkCanvas.h"

namespace formulate {

cass ScrollView {
 public:
  ScrollView(View* child)
    : child_(child) {}
  int Width() const { return size_.width() * scale_; }
  int Height() const { return size_.height() * scale_; }
  void SetSize(const SkSize& size) { size_ = size; }
  void Draw(SkCanvas* canvas);
  void SetScale(float scale) {
    scale_ = scale;
  }
  void SetOrigin(SkPoint origin) { origin_ = origin; }
 private:
  View* child_;
  SkSize size_;
  SkPoint origin_;  // scroll origin; upper left point
  float scale_{1};  // for high-DPI
};

}  // namespace formulate
