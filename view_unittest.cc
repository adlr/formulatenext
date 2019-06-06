// Copyright...

#include <gtest/gtest.h>

#include "view.h"

using namespace formulate;

void SkRectEq(const SkRect& left, const SkRect& right) {
  EXPECT_EQ(left.fLeft, right.fLeft);
  EXPECT_EQ(left.fTop, right.fTop);
  EXPECT_EQ(left.fRight, right.fRight);
  EXPECT_EQ(left.fBottom, right.fBottom);
}

TEST(ViewTest, FrameBoundsTest) {
  View child;
  View parent;
  parent.SetSize(SkSize::Make(10, 10));
  child.SetSize(SkSize::Make(2, 2));
  child.SetOrigin(SkPoint::Make(1, 2));
  child.SetScale(2);
  parent.AddChild(&child);
  EXPECT_EQ(child.Width(), 2);
  SkRectEq(child.Bounds(), SkRect::MakeXYWH(0, 0, 2, 2));
  SkRectEq(child.Frame(), SkRect::MakeXYWH(1, 2, 4, 4));
}

TEST(ViewTest, CoordinateConversionTest) {
  View child;
  View parent;
  parent.SetSize(SkSize::Make(120, 120));
  child.SetSize(SkSize::Make(50, 50));
  child.SetOrigin(SkPoint::Make(10, 10));
  child.SetScale(2);
  parent.AddChild(&child);
  EXPECT_EQ(&parent, child.Parent());

  EXPECT_EQ(parent.ConvertPointToChild(&child, SkPoint::Make(10, 10)),
            SkPoint::Make(0, 0));
  EXPECT_EQ(parent.ConvertPointToChild(&child, SkPoint::Make(20, 20)),
            SkPoint::Make(5, 5));
  EXPECT_EQ(parent.ConvertPointFromChild(&child, SkPoint::Make(0, 0)),
            SkPoint::Make(10, 10));
  EXPECT_EQ(parent.ConvertPointFromChild(&child, SkPoint::Make(5, 5)),
            SkPoint::Make(20, 20));

  EXPECT_EQ(parent.ConvertSizeToChild(&child, SkSize::Make(1, 1)),
            SkSize::Make(0.5, 0.5));
  EXPECT_EQ(parent.ConvertSizeFromChild(&child, SkSize::Make(0.5, 0.5)),
            SkSize::Make(1, 1));

  EXPECT_EQ(parent.ConvertRectToChild(&child, SkRect::MakeXYWH(
      20, 20, 20, 20)),
            SkRect::MakeXYWH(5, 5, 10, 10));
  EXPECT_EQ(parent.ConvertRectFromChild(&child, SkRect::MakeXYWH(
      5, 5, 10, 10)),
            SkRect::MakeXYWH(20, 20, 20, 20));
}

TEST(ViewTest, VisibleSubrectTest) {
  View child;
  View parent;
  parent.SetSize(SkSize::Make(120, 60));
  child.SetSize(SkSize::Make(50, 50));
  child.SetOrigin(SkPoint::Make(10, 10));
  child.SetScale(2);
  parent.AddChild(&child);
  EXPECT_EQ(child.VisibleSubrect(),
            SkRect::MakeXYWH(0, 0, 50, 25));
}
