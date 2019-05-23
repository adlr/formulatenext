// Copyright

#ifndef FORMULATE_SCROLLVIEW_H__
#define FORMULATE_SCROLLVIEW_H__

#include "SkCanvas.h"

#include "view.h"

namespace formulate {

class ScrollView : public View {
 public:
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

  // void MouseDown(SkPoint pt);
  // void MouseDrag(SkPoint pt);
  // void MouseUp(SkPoint pt);
 private:
  bool sent_child_mousedown_{false};
};

}  // namespace formulate

#endif  // FORMULATE_SCROLLVIEW_H__
