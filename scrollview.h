// Copyright

#ifndef FORMULATE_SCROLLVIEW_H__
#define FORMULATE_SCROLLVIEW_H__

#include "SkCanvas.h"

#include "view.h"

namespace formulate {

class ScrollView : public View {
 public:
  explicit ScrollView(bool vertically_center)
      : vertically_center_(vertically_center) {}
  void SetSize(const SkSize& size) {
    View::SetSize(size);
    RepositionChild();
  }
  void SetOrigin(SkPoint origin) {
    origin_ = origin;
    RepositionChild();
  }
  void RepositionChild();

  SkPoint ChildVisibleCenter() const;
  void CenterOnChildPoint(SkPoint point);
 private:
  bool vertically_center_{true};
};

}  // namespace formulate

#endif  // FORMULATE_SCROLLVIEW_H__
