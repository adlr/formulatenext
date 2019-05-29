// Copyright

#include "docview.h"

#include "SkPaint.h"

#include "formulate_bridge.h"

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
  SetNeedsDisplay();
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

SkPoint DocView::ViewPointToPagePoint(const SkPoint& point, int page) {
  // find the page
  float top = kBorderPixels;
  for (int i = 0; i < page; i++)
    top += page_sizes_[i].height() * zoom_ + kBorderPixels;
  // use this page
  float left =
      kBorderPixels + (max_page_width_ - page_sizes_[page].width()) * zoom_ / 2;
  SkPoint ret = SkPoint::Make((point.x() - left) / zoom_,
                              (point.y() - top) / zoom_);
  // flip y on page
  ret.fY = page_sizes_[page].height() - ret.fY;
  return ret;
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
  if (toolbox_.current_tool() == Toolbox::kFreehand_Tool) {
    int page = -1;
    SkPoint pt;
    ViewPointToPageAndPoint(ev.position(), &page, &pt);
    freehand_page_ = page;
    freehand_points_.push_back(pt);
  }
  return this;
}

namespace {
// From https://gist.github.com/njvack/6925609
// |pts| point to an array of 4 points. Control points for the middle
// segment are returned.
// For the first segment, pass the first point twice. For the last segment,
// pass the last point twice.
std::pair<SkPoint, SkPoint> ControlPoints(const SkPoint* pts) {
  std::pair<SkPoint, SkPoint> ret;
  ret.first.fX = (-pts[0].x() + 6*pts[1].x() + pts[2].x()) / 6;
  ret.first.fY = (-pts[0].y() + 6*pts[1].y() + pts[2].y()) / 6;
  ret.second.fX = (pts[1].x() + 6*pts[2].x() - pts[3].x()) / 6;
  ret.second.fY = (pts[1].y() + 6*pts[2].y() - pts[3].y()) / 6;
  return ret;
}
}  // namespace {}

void DocView::MouseDrag(MouseInputEvent ev) {
  if (freehand_page_ >= 0) {
    // continue line drawing
    SkPoint pt = ViewPointToPagePoint(ev.position(), freehand_page_);
    freehand_points_.push_back(pt);
    if (freehand_points_.size() > 2) {
      // Draw up to previous point
      std::pair<SkPoint, SkPoint> ctrlpoints;
      if (freehand_points_.size() < 4) {
        SkPoint input[4] = {
          freehand_points_[0], freehand_points_[0],
          freehand_points_[1], freehand_points_[2]
        };
        ctrlpoints = ControlPoints(input);
      } else {
        ctrlpoints = ControlPoints(
            &freehand_points_[freehand_points_.size() - 4]);
      }
      SkPoint bezier[4] = {
        PagePointToViewPoint(freehand_page_,
                             freehand_points_[freehand_points_.size() - 3]),
        PagePointToViewPoint(freehand_page_, ctrlpoints.first),
        PagePointToViewPoint(freehand_page_, ctrlpoints.second),
        PagePointToViewPoint(freehand_page_,
                             freehand_points_[freehand_points_.size() - 2])
      };
      bridge_drawBezier(this, bezier, zoom_);
    }
  }
}

void DocView::MouseUp(MouseInputEvent ev) {
  int page = 0;
  SkPoint pagept;
  ViewPointToPageAndPoint(ev.position(), &page, &pagept);
  if (editing_text_page_ >= 0) {
    if (!editing_text_str_.empty()) {
      fprintf(stderr, "Commit: %s\n", editing_text_str_.c_str());
      bridge_stopComposingText();
      doc_.PlaceText(editing_text_page_, editing_text_point_,
                     editing_text_str_);
    }
    editing_text_str_.clear();
    editing_text_page_ = -1;
  } else if (toolbox_.current_tool() == Toolbox::kText_Tool) {
    editing_text_page_ = page;
    editing_text_point_ = pagept;
    fprintf(stderr, "edit at %f %f\n", ev.position().x(),
            ev.position().y());
    bridge_startComposingText(ev.position(), this, zoom_);
  } else if (toolbox_.current_tool() == Toolbox::kFreehand_Tool) {
    if (!freehand_points_.empty()) {
      std::vector<SkPoint> bezier_path;
      bezier_path.push_back(freehand_points_[0]);
      for (size_t i = 1; i < freehand_points_.size(); i++) {
        SkPoint input[4] = {
          i == 1 ? freehand_points_[i - 1] : freehand_points_[i - 2],
          freehand_points_[i - 1],
          freehand_points_[i],
          i + 1 == freehand_points_.size() ?
          freehand_points_[i] : freehand_points_[i + 1]
        };
        std::pair<SkPoint, SkPoint> ctrl = ControlPoints(input);
        bezier_path.push_back(ctrl.first);
        bezier_path.push_back(ctrl.second);
        bezier_path.push_back(freehand_points_[i]);
      }
      doc_.InsertFreehandDrawing(freehand_page_, bezier_path);
      SetNeedsDisplay();
      freehand_page_ = -1;
      freehand_points_.clear();
    }
  }
}

}  // namespace formulate
