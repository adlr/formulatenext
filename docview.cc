// Copyright

#include "docview.h"

#include "SkPaint.h"

#include "formulate_bridge.h"

namespace formulate {

char KnobsForType(PDFDoc::ObjType type) {
  switch (type) {
    case PDFDoc::kUnknown:
    case PDFDoc::kShading:
    case PDFDoc::kForm:
      return kNoKnobs;
    case PDFDoc::kText:
      return kMiddleLeftKnob;
    case PDFDoc::kPath:
    case PDFDoc::kImage:
      return kAllKnobs;
  }
}

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
  DrawKnobs(canvas, rect);
}

void DocView::DrawKnobs(SkCanvas* canvas, SkRect rect) {
  if (selected_page_ < 0)
    return;
  SkRect pagerect =
      SkRect::MakeXYWH(0, 0, page_sizes_[selected_page_].width(),
                                     page_sizes_[selected_page_].height());
  if (!rect.intersects(ConvertRectFromPage(selected_page_, pagerect)))
    return;
  SkPaint paint;
  paint.setAntiAlias(true);
  for (int index : selected_objs_) {
    fprintf(stderr, "drawing knobs for %d\n", index);
    SkRect bbox =
        ConvertRectFromPage(selected_page_,
                            doc_.BoundingBoxForObj(selected_page_, index));
    // Draw grey border around object
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(0xffdddddd);  // opaque light grey
    paint.setStrokeWidth(1);
    canvas->drawRect(bbox.makeOutset(0.5, 0.5), paint);

    Knobmask knobs = KnobsForType(doc_.ObjectType(selected_page_, index));
    for (int i = 0; i < 8; i++) {
      Knobmask knob = 1 << i;
      if (knob & knobs) {
        SkRect knobrect = KnobRect(knob, bbox);
        fprintf(stderr, "drawing rect %f %f %f %f\n",
                knobrect.left(), knobrect.top(), knobrect.right(),
                knobrect.bottom());
        // Draw white part
        paint.setStyle(SkPaint::kFill_Style);
        paint.setColor(0xffffffff);  // opaque white
        paint.setStrokeWidth(0);
        canvas->drawRect(knobrect, paint);
        // Draw border
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setColor(0xff000000);  // opaque black
        paint.setStrokeWidth(1);
        canvas->drawRect(knobrect, paint);
      }
    }
  }
}

SkRect DocView::KnobRect(Knobmask knob, SkRect objbounds) {
  float xcenter;
  switch (knob) {
    default:
      fprintf(stderr, "Illegal knob passed 0x%08x\n", knob);
      return SkRect::MakeEmpty();
    case kTopLeftKnob:
    case kMiddleLeftKnob:
    case kBottomLeftKnob:
      xcenter = objbounds.left();
      break;
    case kTopCenterKnob:
    case kBottomCenterKnob:
      xcenter = objbounds.centerX();
      break;
    case kTopRightKnob:
    case kMiddleRightKnob:
    case kBottomRightKnob:
      xcenter = objbounds.right();
      break;
  }

  float ycenter;
  switch (knob) {
    default:
      fprintf(stderr, "Illegal knob passed 0x%08x\n", knob);
      return SkRect::MakeEmpty();
    case kTopLeftKnob:
    case kTopCenterKnob:
    case kTopRightKnob:
      ycenter = objbounds.top();
      break;
    case kMiddleLeftKnob:
    case kMiddleRightKnob:
      ycenter = objbounds.centerY();
      break;
    case kBottomLeftKnob:
    case kBottomCenterKnob:
    case kBottomRightKnob:
      ycenter = objbounds.bottom();
      break;
  }
  float knobwidth = KnobWidth();
  return SkRect::MakeXYWH(xcenter - knobwidth / 2,
                          ycenter - knobwidth / 2,
                          knobwidth,
                          knobwidth);
}

SkRect DocView::KnobBounds(Knobmask knobs, SkRect objbounds) {
  float knob_border_width = KnobBorderWidth();
  SkRect outline_bounds = objbounds.makeOutset(1, 1);
  if (knobs == kMiddleLeftKnob) {
    outline_bounds.join(
        KnobRect(knobs, objbounds).makeOutset(knob_border_width / 2,
                                              knob_border_width / 2));
  } else if (knobs == kAllKnobs) {
    SkRect ret = KnobRect(kTopLeftKnob, objbounds);
    ret.join(KnobRect(kBottomRightKnob, objbounds));
    outline_bounds.join(
        ret.makeOutset(knob_border_width / 2,
                       knob_border_width / 2));
  } else {
    fprintf(stderr, "Please implement better case in KnobBounds()\n");
    SkRect ret = SkRect::MakeEmpty();
    for (int i = 0; i < 8; i++) {
      char knob = 1 << i;
      if (knob & knobs) {
        ret.join(KnobRect(knob, objbounds));
      }
    }
    outline_bounds.join(ret.makeOutset(knob_border_width / 2,
                                       knob_border_width / 2));
  }
  return outline_bounds;
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
  mouse_down_point_ = ev.position();
  mouse_moved_ = false;
  mouse_down_obj_ = -1;
  mouse_down_knob_ = kNoKnobs;
  prev_drag_point_valid_ = false;
  if (toolbox_.current_tool() == Toolbox::kFreehand_Tool) {
    int page = -1;
    SkPoint pt;
    ViewPointToPageAndPoint(ev.position(), &page, &pt);
    freehand_page_ = page;
    freehand_points_.push_back(pt);
  }
  if (toolbox_.current_tool() == Toolbox::kArrow_Tool) {
    int pageno = -1;
    SkPoint pagept = SkPoint::Make(0, 0);
    ViewPointToPageAndPoint(ev.position(), &pageno, &pagept);
    int obj = doc_.ObjectUnderPoint(pageno, pagept, true);
    fprintf(stderr, "Object under pt: (%d, %f %f) %d\n", pageno,
            pagept.x(), pagept.y(), obj);
    if (obj < 0) {
      ClearSelection();
    } else {
      SelectOneObject(pageno, obj);
    }
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
  mouse_moved_ = true;
  
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

  if (mouse_down_knob_ == kNoKnobs &&
      selected_objs_.find(mouse_down_obj_) != selected_objs_.end()) {
    // move the object
    SkPoint prev = mouse_down_point_;
    if (prev_drag_point_valid_)
      prev = prev_drag_point_;
    float dx = (ev.position().x() - prev.x()) / zoom_;
    float dy = (ev.position().y() - prev.y()) / zoom_;
    SetNeedsDisplayInSelection();
    doc_.MoveObjects(selected_page_, selected_objs_, dx, dy);
    SetNeedsDisplayInSelection();
  } else if (mouse_down_knob_) {
    // Move the knob
    SkPoint prev = mouse_down_point_;
    if (prev_drag_point_valid_)
      prev = prev_drag_point_;
    float dx = (ev.position().x() - prev.x()) / zoom_;
    float dy = (ev.position().y() - prev.y()) / zoom_;
    SkRect old_bbox = doc_.BoundingBoxForObj(selected_page_, mouse_down_obj_);
    SkRect new_bbox = GetNewBounds(old_bbox, mouse_down_knob_, dx, dy, true);
    SetNeedsDisplayInObj(selected_page_, mouse_down_obj_);
    doc_.SetObjectBounds(selected_page_, mouse_down_obj_, new_bbox);
    SetNeedsDisplayInObj(selected_page_, mouse_down_obj_);
  }

  prev_drag_point_valid_ = true;
  prev_drag_point_ = ev.position();
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

void DocView::SelectOneObject(int pageno, int index) {
  SetNeedsDisplayInSelection();  // previously selected object(s)
  selected_page_ = pageno;
  selected_objs_.clear();
  selected_objs_.insert(index);
  SetNeedsDisplayInSelection();  // new object
}

void DocView::AddObjectToSelection(int pageno, int index) {
  if (selected_page_ >= 0 && pageno != selected_page_) {
    fprintf(stderr, "Can't add to selection in different page\n");
    return;
  }
  if (selected_objs_.find(index) != selected_objs_.end()) {
    // Already selected
    return;
  }
  selected_objs_.insert(index);
  SetNeedsDisplayInObj(pageno, index);
}

void DocView::UnselectObject(int pageno, int index) {
  if (selected_page_ != pageno) {
    fprintf(stderr, "Invalid page in UnselectObject\n");
    return;
  }
  if (selected_objs_.find(index) == selected_objs_.end()) {
    fprintf(stderr, "Invalid index in UnselectObject\n");
    return;
  }
  SetNeedsDisplayInObj(pageno, index);
  selected_objs_.erase(index);
  if (selected_objs_.empty())
    selected_page_ = -1;
}

void DocView::ClearSelection() {
  SetNeedsDisplayInSelection();
  selected_page_ = -1;
  selected_objs_.clear();
}

void DocView::SetNeedsDisplayInSelection() {
  if (selected_page_ < 0)
    return;
  for (int index : selected_objs_) {
    SetNeedsDisplayInObj(selected_page_, index);
  }
}

void DocView::SetNeedsDisplayInObj(int pageno, int index) {
  if (pageno < 0)
    return;
  SkRect bbox = ConvertRectFromPage(pageno,
                                    doc_.BoundingBoxForObj(pageno, index));
  SkRect full_bounds = KnobBounds(KnobsForType(doc_.ObjectType(pageno, index)),
                                  bbox);
  SetNeedsDisplayInRect(full_bounds);
}

bool DocView::SetKnobUnderPoint(SkPoint viewpt) {
  for (auto it = selected_objs_.rbegin(); it != selected_objs_.rend(); ++it) {
    fprintf(stderr, "set reverse %d\n", *it);
    SkRect bbox = doc_.BoundingBoxForObj(selected_page_, *it);
    for (int i = 0; i < 8; i++) {
      Knobmask knob = 1 << i;
      SkRect knobrect = KnobRect(knob, bbox);
      if (knobrect.contains(viewpt.x(), viewpt.y())) {
        mouse_down_obj_ = *it;
        mouse_down_knob_ = knob;
        return true;
      }
    }
  }
  return false;
}

SkRect DocView::GetNewBounds(SkRect old_bounds, Knobmask knob,
                             float dx, float dy, bool freeform) {
  if (knob & (kTopCenterKnob | kMiddleLeftKnob |
              kMiddleRightKnob | kBottomCenterKnob)) {
    if (knob == kTopCenterKnob)
      old_bounds.fTop += dy;
    if (knob == kMiddleLeftKnob)
      old_bounds.fLeft += dx;
    if (knob == kMiddleRightKnob)
      old_bounds.fRight += dx;
    if (knob == kBottomCenterKnob)
      old_bounds.fBottom += dy;
    return old_bounds;
  }

  // Really narrow objects are automatically freeform
  if (!freeform && (old_bounds.width() > 0.1) && (old_bounds.height() > 0.1)) {
    // For now, we use dx to decide how to transform
    dy = dx * old_bounds.height() / old_bounds.width();
    if (knob & (kTopRightKnob | kBottomLeftKnob)) {
      dy *= -1;
    }
  }

  if (knob & (kTopLeftKnob | kTopRightKnob))
    old_bounds.fTop += dy;
  if (knob & (kBottomLeftKnob | kBottomRightKnob))
    old_bounds.fBottom += dy;
  if (knob & (kTopLeftKnob | kBottomLeftKnob))
    old_bounds.fLeft += dx;
  if (knob & (kTopRightKnob | kBottomRightKnob))
    old_bounds.fRight += dx;
  return old_bounds;
}

}  // namespace formulate
