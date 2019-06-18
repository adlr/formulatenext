// Copyright...

#include <gtest/gtest.h>

#include "docview.h"

namespace formulate {

TEST(DocViewTest, KnobsTest) {
  SkRect obj = SkRect::MakeXYWH(10, 10, 20, 20);
  EXPECT_EQ(SkRect::MakeXYWH(7, 17, 6, 6),
            DocView::KnobRect(kMiddleLeftKnob, obj));
  EXPECT_EQ(SkRect::MakeXYWH(6.5, 16.5, 7, 7),
            DocView::KnobBounds(kMiddleLeftKnob, obj));
  EXPECT_EQ(SkRect::MakeLTRB(6.5, 6.5, 33.5, 33.5),
            DocView::KnobBounds(kAllKnobs, obj));
  EXPECT_EQ(SkRect::MakeLTRB(6.5, 6.5, 33.5, 33.5),
            DocView::KnobBounds(
                kTopLeftKnob | kTopRightKnob | kBottomCenterKnob, obj));
}

TEST(DocViewTest, KnobTypeMaskTest) {
  EXPECT_EQ(kNoKnobs, KnobsForType(PDFDoc::kUnknown));
  EXPECT_EQ(kMiddleLeftKnob, KnobsForType(PDFDoc::kText));
  EXPECT_EQ(kAllKnobs, KnobsForType(PDFDoc::kPath));
}

}  // namespace formulate
