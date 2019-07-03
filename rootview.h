// Copyright...

#ifndef ROOTVIEW_H_
#define ROOTVIEW_H_

#include "view.h"

namespace formulate {

class ScopedRedraw;

class RootView : public View {
 public:
  explicit RootView(int id) : id_(id) {}
  virtual const char* Name() const { return "RootView"; }
  void SetNeedsDisplayInRect(const SkRect& rect);
  void SetRedraw(ScopedRedraw* redraw);
  void DoDraw(SkRect rect);
  void SetBgColor(uint32_t color) { bg_color_ = color; }

  View* MouseDown(MouseInputEvent ev);
  void MouseDrag(MouseInputEvent ev);
  void MouseUp(MouseInputEvent ev);

 private:
  ScopedRedraw* redraw_{nullptr};
  int id_{-1};
  uint32_t bg_color_{0x000000};  // transparent black
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
