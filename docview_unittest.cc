// Copyright...

#include <gtest/gtest.h>

#include "docview.h"

namespace formulate {

TEST(DocViewTest, KnobsTest) {
  DocView doc;
  SkRect obj = SkRect::MakeXYWH(10, 10, 20, 20);
  float knob_width = DocView::KnobWidth();
  float border_width = DocView::KnobBorderWidth();
  float line_width = 1;
  SkRect outline_bounds = obj.makeOutset(line_width, line_width);
  EXPECT_EQ(SkRect::MakeXYWH(10 - knob_width / 2,
                             20 - knob_width / 2,
                             knob_width,
                             knob_width),
            doc.KnobRect(kMiddleLeftKnob, obj));
  {
    SkRect temp_bounds = outline_bounds;
    temp_bounds.join(SkRect::MakeXYWH(10 - knob_width / 2 - border_width / 2,
                                      20 - knob_width / 2 - border_width / 2,
                                      knob_width + border_width,
                                      knob_width + border_width));
    EXPECT_EQ(temp_bounds, doc.KnobBounds(kMiddleLeftKnob, obj));
  }
  {
    SkRect temp_bounds = outline_bounds;
    temp_bounds.join(SkRect::MakeLTRB(10 - knob_width / 2 - border_width / 2,
                                      10 - knob_width / 2 - border_width / 2,
                                      30 + knob_width / 2 + border_width / 2,
                                      30 + knob_width / 2 + border_width / 2));
    EXPECT_EQ(temp_bounds, doc.KnobBounds(kAllKnobs, obj));
  }
  {
    SkRect temp_bounds = outline_bounds;
    temp_bounds.join(SkRect::MakeLTRB(10 - knob_width / 2 - border_width / 2,
                                      10 - knob_width / 2 - border_width / 2,
                                      30 + knob_width / 2 + border_width / 2,
                                      30 + knob_width / 2 + border_width / 2));
    EXPECT_EQ(temp_bounds, doc.KnobBounds(kAllKnobs, obj));
    EXPECT_EQ(temp_bounds,
              doc.KnobBounds(kTopLeftKnob | kTopRightKnob | kBottomCenterKnob,
                             obj));
  }
}

TEST(DocViewTest, KnobTypeMaskTest) {
  EXPECT_EQ(kNoKnobs, KnobsForType(PDFDoc::kUnknown));
  EXPECT_EQ(kMiddleLeftKnob, KnobsForType(PDFDoc::kText));
  EXPECT_EQ(kAllKnobs, KnobsForType(PDFDoc::kPath));
}

}  // namespace formulate
