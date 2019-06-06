// Copyright...

#include <gtest/gtest.h>

#include "rootview.h"

using namespace formulate;

class DrawRecordView : public View {
 public:
  void Draw(SkCanvas* canvas, SkRect rect) {
    EXPECT_FALSE(drawn_);
    drawn_ = true;
    draw_rect_ = rect;
  }
  SkRect draw_rect_;
  bool drawn_{false};
};

TEST(RootViewTest, UnnecessaryDrawTest) {
  DrawRecordView child;
  RootView parent(0);
  parent.SetSize(SkSize::Make(100, 100));
  child.SetOrigin(SkPoint::Make(0, 10));
  child.SetSize(SkSize::Make(100, 100));
  parent.AddChild(&child);
  {
    ScopedRedraw redraw(&parent);
    child.SetNeedsDisplayInRect(SkRect::MakeXYWH(10, 92, 2, 2));
  }
  EXPECT_FALSE(child.drawn_);
}

TEST(RootViewTest, PartialDrawTest) {
  DrawRecordView child;
  RootView parent(0);
  parent.SetSize(SkSize::Make(100, 100));
  child.SetOrigin(SkPoint::Make(0, 10));
  child.SetSize(SkSize::Make(100, 100));
  parent.AddChild(&child);
  {
    ScopedRedraw redraw(&parent);
    child.SetNeedsDisplayInRect(SkRect::MakeXYWH(10, 80, 2, 20));
  }
  EXPECT_TRUE(child.drawn_);
  EXPECT_EQ(child.draw_rect_, SkRect::MakeXYWH(10, 80, 2, 10));
}
