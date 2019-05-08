// Copyright

#include "docview.h"

#include "SkPaint.h"

namespace formulate {

namespace {
  float kBorderPixels = 20.0;
}

float DocView::Width() const {
  return MaxPageWidth() * zoom_ + kBorderPixels * 2;
}

float DocView::Height() const {
  float ret = kBorderPixels;
  for (auto it = page_sizes_.begin(); it != page_sizes_.end(); ++it)
    ret += it->height() * zoom_ + kBorderPixels;
  return ret;
}

void DocView::Draw(SkCanvas* canvas, SkRect rect) {
  SkPaint paint;

  // Fill view by default with grey
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(0xff808080);  // opaque grey
  paint.setStrokeWidth(0);
  canvas->drawRect(rect, paint);

  // Draw a rectangle for each page and paint each page
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setColor(0xff000000);  // opaque black
  paint.setStrokeWidth(1);
  float pagetop = kBorderPixels;
  for (int i = 0; i < page_sizes_.size(); i++) {
    const SkSize& pgsize = page_sizes_[i];
    const float pageleft = kBorderPixels +
      floorf((max_page_width_ - pgsize.width()) / 2);
    SkRect page = SkRect::MakeXYWH(pageleft, pagetop,
				   pgsize.width() * zoom_,
                                   pgsize.height() * zoom_);
    SkRect pageBorder = page.makeInset(-0.5, -0.5);
    SkRect pageBorderBorder = pageBorder.makeInset(-0.5, -0.5);
    if (SkRect::Intersects(rect, pageBorderBorder)) {
      // draw black border
      paint.setStyle(SkPaint::kStroke_Style);
      paint.setColor(0xff000000);  // opaque black
      paint.setStrokeWidth(1);
      canvas->drawRect(pageBorder, paint);
    }

    SkRect pagePaint = rect;
    if (pagePaint.intersect(page)) {
      // paint page white
      paint.setStyle(SkPaint::kFill_Style);
      paint.setColor(0xffffffff);  // opaque white
      paint.setStrokeWidth(0);
      canvas->drawRect(page, paint);

      canvas->save();

      canvas->translate(pageleft, pagetop);
      canvas->scale(zoom_, zoom_);
      
      SkRect pageDrawClip =
        SkRect::MakeXYWH((pagePaint.fLeft - pageleft) / zoom_,
                         (pagePaint.fTop - pagetop) / zoom_,
                         pagePaint.width() / zoom_,
                         pagePaint.height() / zoom_);
      doc_.DrawPage(canvas, pageDrawClip, static_cast<int>(i));

      canvas->restore();
    }
    pagetop += pgsize.height() + kBorderPixels;
  }
}

void DocView::RecomputePageSizes() {
  page_sizes_.resize(doc_.Pages());
  max_page_width_ = 0.0;
  for (int i = 0; i < page_sizes_.size(); i++) {
    page_sizes_[i] = doc_.PageSize(i);
    max_page_width_ = std::max(max_page_width_, page_sizes_[i].width());
  }
}

}  // namespace formulate
