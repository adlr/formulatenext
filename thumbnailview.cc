// Copyright

#include "thumbnailview.h"

#include "SkFont.h"
#include "SkTextUtils.h"

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
  for (int i = 0; i < pages; i++) {
    SkRect pagerect = SkRect::MakeXYWH(4, 8 + top, pagesize, pagesize);
    SkSize pdfsize = doc_->PageSize(i);
    // Avoid divide by 0
    pdfsize.fWidth = std::max(0.1f, pdfsize.fWidth);
    pdfsize.fHeight = std::max(0.1f, pdfsize.fHeight);
    // Reduce pagerect on either x or y to make proportional to pdfsize
    float scale = std::min(pagerect.width() / pdfsize.width(),
                           pagerect.height() / pdfsize.height());
    float dx = (pagesize - (scale * pdfsize.width())) / 2;
    float dy = (pagesize - (scale * pdfsize.height())) / 2;
    pagerect.inset(dx, dy);
    SkRect pageborder = pagerect.makeOutset(0.5, 0.5);
    if (pageborder.intersects(rect)) {
      paint.setStyle(SkPaint::kStroke_Style);
      paint.setStrokeWidth(1);
      paint.setColor(0xff000000);  // opaque black
      canvas->drawRect(pageborder, paint);
    }
    if (pagerect.intersects(rect)) {
      // paint page white
      paint.setStyle(SkPaint::kFill_Style);
      paint.setColor(0xffffffff);  // opaque white
      paint.setStrokeWidth(0);
      canvas->drawRect(pagerect, paint);

      // Draw PDF page
      canvas->save();
      canvas->translate(pagerect.left(), pagerect.top());
      canvas->scale(scale, scale);
      SkRect pdfrect =
          SkRect::MakeXYWH(0, 0, pdfsize.width(), pdfsize.height());
      doc_->DrawPage(canvas, pdfrect, i);
      canvas->restore();
    }
    char pagenumber[50];
    if (snprintf(pagenumber, sizeof(pagenumber), "%d", i + 1) < 0) {
      fprintf(stderr, "snprintf error!\n");
      return;
    }
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setStrokeWidth(0);
    paint.setColor(0xff000000);  // opaque black
    SkTextUtils::DrawString(canvas, pagenumber,
                            4 + pagesize / 2, 8 + top + pagesize + 4 + 12,
                            SkFont(), paint, SkTextUtils::kCenter_Align);
    top += pagesize + 12 + 8 + 5;
  }
}

void ThumbnailView::SelectPage(int page) {
  if (page >= doc_->Pages()) {
    fprintf(stderr, "too high of page to select\n");
    return;
  }
  last_selected_page_ = page;
  for (int i = 0; i < selected_pages_.size(); i++) {
    if (selected_pages_[i] && i != page) {
      // Deselect page
      selected_pages_[i] = false;
      SetNeedsDisplayForPage(i);
    }
    if (!selected_pages_[i] && i == page) {
      // Select page
      selected_pages_[i] = true;
      SetNeedsDisplayForPage(i);
    }
  }
}

void ThumbnailView::TogglePageSelected(int page) {
  if (page >= doc_->Pages()) {
    fprintf(stderr, "too high of page to select\n");
    return;
  }
  selected_pages_[page] = ! selected_pages_[page];
  SetNeedsDisplayForPage(page);
  if (!selected_pages_[page] && last_selected_page_ == page) {
    // User deselected the last selected page. Move to last selected page
    last_selected_page_ = -1;
    for (int i = selected_pages_.size() - 1; i >= 0; i--) {
      if (!selected_pages_[i])
        continue;
      last_selected_page_ = i;
      break;
    }
  }
}

void ThumbnailView::SelectToPage(int page) {
  if (page >= doc_->Pages()) {
    fprintf(stderr, "too high of page to select\n");
    return;
  }
  int first = std::max(0, std::min(last_selected_page_, page));
  int last = std::max(last_selected_page_, page);
  last_selected_page_ = page;
  for (int i = first; i <= last; i++) {
    if (selected_pages_[i])
      continue;  // already selected
    selected_pages_[i] = true;
    SetNeedsDisplayForPage(i);
  }
}

void ThumbnailView::PagesChanged() {
  selected_pages_.resize(doc_->Pages());
  if (last_selected_page_ >= doc_->Pages())
    last_selected_page_ = -1;
  SetNeedsDisplay();
}

void ThumbnailView::NeedsDisplayInRect(int page, SkRect rect) {
  // TODO(adlr): be more efficient
  SetNeedsDisplay();
}

}  // namespace formulate
