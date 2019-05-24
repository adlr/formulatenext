// Copyright

#include "docview.h"

#include "SkPaint.h"

namespace formulate {

namespace {
  float kBorderPixels = 20.0;
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
      floorf((max_page_width_ - pgsize.width()) * zoom_ / 2);
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
    pagetop += pgsize.height() * zoom_ + kBorderPixels;
  }
}

void DocView::SetZoom(float zoom) {
  zoom_ = zoom;
  RecomputePageSizes();
}

void DocView::RecomputePageSizes() {
  page_sizes_.resize(doc_.Pages());
  max_page_width_ = 0.0;
  float height = kBorderPixels;
  for (int i = 0; i < page_sizes_.size(); i++) {
    page_sizes_[i] = doc_.PageSize(i);
    max_page_width_ = std::max(max_page_width_, page_sizes_[i].width());
    height += page_sizes_[i].height() * zoom_ + kBorderPixels;
  }
  SetSize(SkSize::Make(max_page_width_ * zoom_ + kBorderPixels * 2,
                       height));
}

void DocView::ViewPointToPageAndPoint(const SkPoint& viewpt,
                                      int* out_page,
                                      SkPoint* out_pagept) const {
  // find best page
  float bottom = kBorderPixels / 2;
  int i;
  for (i = 0; i < page_sizes_.size(); i++) {
    bottom += page_sizes_[i].height() * zoom_ + kBorderPixels;
    if (viewpt.y() < bottom || i == page_sizes_.size() - 1)
      break;
  }
  // use this page
  float top = bottom - page_sizes_[i].height() * zoom_ - kBorderPixels / 2;
  float left =
    kBorderPixels + (max_page_width_ - page_sizes_[i].width()) * zoom_ / 2;
  *out_page = i;
  *out_pagept = SkPoint::Make((viewpt.x() - left) / zoom_,
                              (viewpt.y() - top) / zoom_);
  // flip y on page
  out_pagept->fY = page_sizes_[i].height() - out_pagept->fY;
}

SkPoint DocView::PagePointToViewPoint(int page, const SkPoint& pagept) const {
  if (page >= page_sizes_.size()) {
    fprintf(stderr, "PagePointToViewPoint: invalid page!\n");
    return SkPoint();
  }
  float top = kBorderPixels;
  for (int i = 0; i < page; i++) {
    top += page_sizes_[i].height() * zoom_ + kBorderPixels;
  }
  float left =
    kBorderPixels + (max_page_width_ - page_sizes_[page].width()) / 2 * zoom_;
  // flip y back on page, but not x
  return SkPoint::Make(pagept.x() * zoom_ + left,
                       (page_sizes_[page].height() - pagept.y()) * zoom_ + top);
}

SkRect DocView::ConvertRectFromPage(int page, const SkRect& rect) const {
  SkPoint topleft = SkPoint::Make(rect.left(), rect.top());
  SkPoint botright = SkPoint::Make(rect.right(), rect.bottom());
  topleft = PagePointToViewPoint(page, topleft);
  botright = PagePointToViewPoint(page, botright);
  // Note we are flipping y coordinate in this transform
  if (!rect.isEmpty()) {
    if (topleft.y() < botright.y()) {
      fprintf(stderr, "failed to flip y coordinate!\n");
    }
  }
  return SkRect::MakeLTRB(topleft.x(), botright.y(),
                          botright.x(), topleft.y());
}

View* DocView::MouseDown(MouseInputEvent ev) {
  return this;
}

void DocView::MouseUp(MouseInputEvent ev) {
  int page = 0;
  SkPoint pagept;
  ViewPointToPageAndPoint(ev.position(), &page, &pagept);
  if (toolbox_.current_tool() == Toolbox::kFreehand_Tool) {
    doc_.ModifyPage(page, pagept);
  }
  if (editing_text_page_ >= 0) {
    if (!editing_text_str_.empty()) {
      fprintf(stderr, "Commit: %s\n", editing_text_str_.c_str());
      bridge_stopComposingText();
      SkRect dirty =
          doc_.PlaceText(editing_text_page_, editing_text_point_,
                         editing_text_str_);
      SetNeedsDisplayInRect(ConvertRectFromPage(editing_text_page_, dirty));
    }
    editing_text_str_.clear();
    editing_text_page_ = -1;
  } else if (toolbox_.current_tool() == Toolbox::kText_Tool) {
    editing_text_page_ = page;
    editing_text_point_ = pagept;
    fprintf(stderr, "edit at %f %f\n", ev.position().x(),
            ev.position().y());
    bridge_startComposingText(ev.position(), this, zoom_);
  }
}

}  // namespace formulate
