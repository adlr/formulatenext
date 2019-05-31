// Copyright

#include "thumbnailview.h"

#include "SkFont.h"

namespace formulate {

void ThumbnailView::SetWidth(float width) {
  float pagesize = width - 8;
  SetSize(SkSize::Make(width, (pagesize + 12 + 8 + 5) * doc_->Pages()));
}

void ThumbnailView::Draw(SkCanvas* canvas, SkRect rect) {
  const int pages = doc_->Pages();
  float top = 0.0f;
  float pagesize = Width() - 8;
  SkPaint paint;
  paint.setColor(0xff000000);  // opaque black
  for (int i = 0; i < pages; i++) {
    SkRect pagerect = SkRect::MakeXYWH(4, 8 + top, pagesize, pagesize);
    SkRect pageborder = pagerect.makeOutset(1, 1);
    if (pageborder.intersects(rect)) {
      paint.setStyle(SkPaint::kStroke_Style);
      paint.setStrokeWidth(1);
      canvas->drawRect(pageborder, paint);
    }
    char pagenumber[50];
    if (snprintf(pagenumber, sizeof(pagenumber), "%d", i) < 0) {
      fprintf(stderr, "snprintf error!\n");
      return;
    }
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setStrokeWidth(0);
    canvas->drawSimpleText(pagenumber, strlen(pagenumber),
                           SkTextEncoding::kUTF8,
                           4, 8 + top + pagesize + 4 + 12,
                           SkFont(), paint);
    top += pagesize + 12 + 8 + 5;
  }
}

}  // namespace formulate
