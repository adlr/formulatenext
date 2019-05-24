// Copyright...

#ifndef ROOTVIEW_H_
#define ROOTVIEW_H_

#include "view.h"

namespace formulate {

class ScopedRedraw;

class RootView : public View {
 public:
  void SetNeedsDisplayInRect(const SkRect& rect);
  void SetRedraw(ScopedRedraw* redraw);
  void DoDraw(SkRect rect);

  View* MouseDown(MouseInputEvent ev);
  void MouseDrag(MouseInputEvent ev);
  void MouseUp(MouseInputEvent ev);

 private:
  ScopedRedraw* redraw_{nullptr};
};

class ScopedRedraw {
 public:
  explicit ScopedRedraw(RootView* view)
      : view_(view) {
    if (!view_) {
      fprintf(stderr, "Invalid View!\n");
      return;
    }
    view_->SetRedraw(this);
  }
  ~ScopedRedraw();
  void AddRect(const SkRect& rect) { rect_.join(rect); }
 private:
  RootView* view_{nullptr};
  SkRect rect_{SkRect::MakeEmpty()};  // what portion to redraw
};

}  // namespace formulate

#endif  // ROOTVIEW_H_
