// Copyright

#ifndef FORMULATE_SCROLLVIEW_H__
#define FORMULATE_SCROLLVIEW_H__

#include "SkCanvas.h"

#include "view.h"

namespace formulate {

class ScrollView {
 public:
  ScrollView(View* child)
    : child_(child) {}
  float Width() const { return size_.width() * scale_; }
  float Height() const { return size_.height() * scale_; }
  void SetSize(const SkSize& size) {
    size_ = size;
    RepositionChild();
  }
  void Draw(SkCanvas* canvas);
  void SetScale(float scale) {
    scale_ = scale;
  }
  void SetOrigin(SkPoint origin) {
    origin_ = origin;
    RepositionChild();
  }
  void RepositionChild();
  SkPoint ChildVisibleCenter() const;
  void CenterOnChildPoint(SkPoint point);
  void DoDraw();
 private:
  View* child_;
  SkSize size_;
  SkPoint origin_;  // scroll origin; upper left point
  float scale_{1};  // for high-DPI
};

}  // namespace formulate

#endif  // FORMULATE_SCROLLVIEW_H__
