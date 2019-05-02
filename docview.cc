// Copyright

#include "docview.h"

#include "SkPaint.h"

namespace formulate {

namespace {
  float kPageWidth = 8.5 * 72;
  float kPageHeight = 11 * 72;
  float kBorderPixels = 20.0;
}

int DocView::Width() const {
  return kPageWidth * zoom_ + kBorderPixels * 2;
}

int DocView::Height() const {
  return (kPageHeight + kBorderPixels) * pages_ + kBorderPixels;
}

void DocView::Draw(SkCanvas* canvas) {
  canvas->scale(scale_, scale_);
  SkRect rect = SkRect::MakeXYWH(0, 0, Width(), Height());
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(0xff808080);  // opaque grey
  paint.setStrokeWidth(0);
  canvas->drawRect(rect, paint);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setColor(0xff000000);  // opaque black
  paint.setStrokeWidth(1);
  float pagetop = kBorderPixels;
  const float pagewidth = kPageWidth * zoom_;
  const float pageheight = kPageHeight * zoom_;
  for (int i = 0; i < pages_; i++) {
    SkRect page = SkRect::MakeXYWH(kBorderPixels,
				   pagetop,
				   pagewidth, pageheight);
    //canvas->save();
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(0xff000000);  // opaque black
    paint.setStrokeWidth(1);
    canvas->drawRect(page.makeInset(-0.5, -0.5), paint);

    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(0xffffffff);  // opaque white
    paint.setStrokeWidth(0);
    canvas->drawRect(page, paint);
    //canvas->restore();
    pagetop += pageheight + kBorderPixels;
  }
}

}  // namespace formulate
