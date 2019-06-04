// Copyright

#include "thumbnailview.h"

#include "SkFont.h"
#include "SkTextUtils.h"

namespace formulate {

namespace {

/* Visual layout in pixels from top to bottom:
 * kTop* = top of view
 * kPage* = repeated per page
 * kBottom* = bottom of view
 */
const float kTopPadding = 2.0f;
const float kPageHorizCursor = 2.0f;
const float kPageTopSpacing = 2.0f;
const float kPageTopBorder = 2.0f;
const float kPageBottomBorder = kPageTopBorder;
const float kPageNumberSpacing = 2.0f;
const float kPageNumberHeight = 12.0f;
const float kPageBottomSpacing = 2.0f;
const float kBottomHorizCursor = kPageHorizCursor;
const float kBottomPadding = kTopPadding;

// Horizontal visual layout:
const float kLeftPadding = 2.0f;
const float kPageLeftBorder = kPageTopBorder;
const float kPageRightBorder = kPageLeftBorder;
const float kRightPadding = kLeftPadding;

// The following functions are helpers for accessing details of the
// layout specified in the above comment.

float PageBodyWidth(float view_width) {
  // Page body width and height are the same, as we fit the actual
  // page into the given square.
  return view_width -
      (kLeftPadding +
       kPageLeftBorder +
       kPageRightBorder +
       kRightPadding);
}

// Finds out how tall each individual page is (including page number)
// if this view has a given width.
float PageHeightForViewWidth(float view_width) {
  return (kPageHorizCursor +
          kPageTopSpacing +
          kPageTopBorder +
          PageBodyWidth(view_width) +
          kPageBottomBorder +
          kPageNumberSpacing +
          kPageNumberHeight +
          kPageBottomSpacing);
}

float ViewHeightForWidth(float view_width, int num_pages) {
  return kTopPadding +
      num_pages * PageHeightForViewWidth(view_width) +
      kBottomHorizCursor +
      kBottomPadding;
}

SkRect PageBodySquare(int page, float view_width) {
  float page_body_width = PageBodyWidth(view_width);
  float top = kTopPadding +
      PageHeightForViewWidth(view_width) * page +
      kPageHorizCursor +
      kPageTopSpacing +
      kPageTopBorder;
  return SkRect::MakeXYWH(kLeftPadding + kPageLeftBorder, top,
                          page_body_width, page_body_width);
}

SkPoint PageLabelPoint(int page, float view_width) {
  float page_body_width = PageBodyWidth(view_width);
  float top = kTopPadding +
      PageHeightForViewWidth(view_width) * page +
      kPageHorizCursor +
      kPageTopSpacing +
      kPageTopBorder +
      page_body_width +  // page body height (same as width)
      kPageBottomBorder +
      kPageNumberSpacing +
      kPageNumberHeight;
  return SkPoint::Make(view_width / 2, top);
}

}  // namespace {}

void ThumbnailView::SetWidth(float width) {
  SetSize(SkSize::Make(width, ViewHeightForWidth(width, doc_->Pages())));
}

int ThumbnailView::PageForPoint(SkPoint pt) const {
  int pages = doc_->Pages();
  float top = kTopPadding;
  float dy = PageHeightForViewWidth(Width());
  for (int i = 0; i < pages; i++) {
    if (pt.y() < (top + dy))
      return i;
    top += dy;
  }
  return pages - 1;
}

void ThumbnailView::Draw(SkCanvas* canvas, SkRect rect) {
  const int pages = doc_->Pages();
  float pagesize = PageBodyWidth(Width());
  SkPaint paint;
  for (int i = 0; i < pages; i++) {
    SkRect pagerect = PageBodySquare(i, Width());
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
    if (PageIsSelected(i)) {
      pageborder = pageborder.makeOutset(0.5, 0.5);
      paint.setColor(0xff2e75e8);  // opaque black
      paint.setStrokeWidth(2);
    } else {
      paint.setColor(0xff000000);  // opaque black
      paint.setStrokeWidth(1);
    }
    if (pageborder.intersects(rect)) {
      paint.setStyle(SkPaint::kStroke_Style);
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
    SkPoint label_point = PageLabelPoint(i, Width());
    SkTextUtils::DrawString(canvas, pagenumber,
                            label_point.x(), label_point.y(),
                            SkFont(), paint, SkTextUtils::kCenter_Align);
  }
}

void ThumbnailView::OnClick(MouseInputEvent ev) {
  int page = PageForPoint(ev.position());
  if (ev.modifiers() & kShiftKey) {
    SelectToPage(page);
  } else if (ev.modifiers() & kControlKey) {
    TogglePageSelected(page);
  } else {
    SelectPage(page);
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
  if (handler_)
    handler_->ScrollToPage(page);
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
  SetWidth(Width());  // forces height to update
  SetNeedsDisplay();
}

void ThumbnailView::NeedsDisplayInRect(int page, SkRect rect) {
  // TODO(adlr): be more efficient
  SetNeedsDisplay();
}

}  // namespace formulate
