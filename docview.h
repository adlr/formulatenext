// Copyright

#ifndef FORMULATE_DOCVIEW_H__
#define FORMULATE_DOCVIEW_H__

#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "SkCanvas.h"

#include "annotation.h"
#include "pdfdoc.h"
#include "toolbox.h"
#include "view.h"

namespace formulate {

char KnobsForType(PDFDoc::ObjType type);

class DocView : public View,
                public PDFDocEventHandler {
 public:
  DocView() { doc_.AddEventHandler(this); }
  virtual const char* Name() const { return "DocView"; }
  void Draw(SkCanvas* canvas, SkRect rect);
 private:
  static float KnobWidth() { return 9.0f; }
  static float KnobBorderWidth() { return 1.0f; }
  // Assums we're drawing only the one page, with the subrect |rect|
  void DrawKnobs(SkCanvas* canvas, SkRect rect);
  // Returns the rect (not including border) for the given |knob|
  // of the given |objbounds|. Returned rect and |objbounds| are in
  // view coordinates.
  SkRect KnobRect(Knobmask knob, SkRect objbounds);
  // Returns the bounding box of the knobs (including border) for the
  // given |knobs| around the given |objbounds|. Returned rect and
  // |objbounds| are in view coordinates.
  SkRect KnobBounds(Knobmask knobs, SkRect objbounds);
 public:
  void SetZoom(float zoom);

  void RecomputePageSizes();
  float PageWidth(size_t page) const { return page_sizes_[page].width(); }
  float PageHeight(size_t page) const { return page_sizes_[page].height(); }
  float MaxPageWidth() const { return max_page_width_; }

  void VisibleCenterPagePoint(int* out_pg, SkPoint* out_pt) const {
    SkRect rect = VisibleSubrect();
    return ViewPointToPageAndPoint(SkPoint::Make(rect.centerX(),
                                                 rect.centerY()),
                                   out_pg, out_pt);
  }

  // if |pt|, which is in page coordinates, it outside |page|, it is
  // modified to be within page boundaries. The new value (which may be
  // the same as the old value) is returned.
  SkPoint ClampToPage(int page, SkPoint pt) const;

  // Convert point in view coords (within Width() and Height()) to a
  // page and point within that page. The point within that page is in PDF
  // coordinates, so it doesn't take zoom into acct.
  void ViewPointToPageAndPoint(const SkPoint& viewpt,
                               int* out_page,
                               SkPoint* out_pagept) const;
  // Convert 'point' in this' view coordinate space to a point in page's
  // coordinate space
  SkPoint ViewPointToPagePoint(const SkPoint& point, int page);

  // Converts a point in PDF coordinates of a given page to view coords.
  // The inverse of ViewPointToPageAndPoint().
  SkPoint PagePointToViewPoint(int page, const SkPoint& pagept) const;
  SkRect ConvertRectFromPage(int page, const SkRect& rect) const;

  View* MouseDown(MouseInputEvent ev);
  void MouseDrag(MouseInputEvent ev);
  void MouseUp(MouseInputEvent ev);

  // These trigger redraw requests
  // void SelectOneObject(int pageno, int index);
  // void AddObjectToSelection(int pageno, int index);
  // void UnselectObject(int pageno, int index);
  // void ClearSelection();
  // void SetNeedsDisplayInSelection();

  void InsertSignature(const char* svgpath);

  // Returns true if a knob was hit. If a knob was hit, sets member variables.
  // Otherwise, members are left unchanged.
  bool SetKnobUnderPoint(SkPoint pagept);

  // Returns the new bounding box given a drag of |dx|, |dy| on the given knob.
  // If freeform is false and a corner is dragged, the new bounds will be
  // proportional to the old bounds.
  SkRect GetNewBounds(SkRect old_bounds, Knobmask knob, float dx, float dy,
                      bool freeform);

  void StartEditing(int page, SkPoint pagept, TextAnnotation* annot);
  void StopEditing();

  void StartComposingText(int page,
                          SkPoint pt,  // top-left corner
                          float width,  // width, or 0 for bound text
                          const char* html,  // body text
                          int cursorpos);  // where to put cursor
  void StopEditingText();

  PDFDoc doc_;
  Toolbox toolbox_;

  void SetEditingString(const char* str);

  // PDFDocEventHandler methods
  void PagesChanged() {
    RecomputePageSizes();
  }
  void NeedsDisplayInRect(int page, SkRect rect) {
    SetNeedsDisplayInRect(ConvertRectFromPage(page, rect));
  }
  // void NeedsDisplayForObj(int pageno, int index) {
  //   SetNeedsDisplayInObj(pageno, index);
  // }
  // bool FlushAnnotations(FPDF_DOCUMENT doc,
  //                       FPDF_PAGE page,
  //                       int pageno);

  // if |index| is -1, redraw whole page. Includes knobs.
  // void SetNeedsDisplayInObj(int pageno, int index);

  void SetNeedsDisplayForAnnotation(int pageno, Annotation* annot) {
    NeedsDisplayInRect(pageno, annot->Bounds());
  }

 private:
  bool AnnotationIsSelected(Annotation* annot) const {
    return selected_annotations_.find(annot) != selected_annotations_.end();
  }
  void ToggleAnnotationSelected(int page, Annotation* annot);

  std::vector<SkSize> page_sizes_;  // in PDF points
  float max_page_width_{0};  // in PDF points
  float zoom_{1};  // user zoom in/out

  // std::vector<std::unique_ptr<Annotation>> annotations_;
  std::set<Annotation*> selected_annotations_;
  int selected_annotations_page_{-1};
  TextAnnotation* editing_annotation_{nullptr};
  std::unique_ptr<Annotation> placing_annotation_;
  int placing_annotation_page_{-1};

  // Move annotations intermediate data
  bool dragging_{false};
  SkPoint drag_start_;  // in page coords
  SkPoint last_drag_pt_;

  // TODO Delete these:

  int editing_text_page_{-1};
  SkPoint editing_text_point_;  // in PDF points
  std::string editing_text_str_;
  int editing_text_obj_{-1};
  std::string editing_text_orig_value_;  // when editing existing

  int freehand_page_{-1};
  std::vector<SkPoint> freehand_points_;
  int freehand_merge_obj_index_{-1};

  SkPoint mouse_down_point_;
  bool mouse_moved_{false};
  bool prev_drag_point_valid_{false};
  SkPoint prev_drag_point_;

  // Selected objects
  int selected_page_{-1};
  std::set<int> selected_objs_;

  int mouse_down_obj_{-1};  // one of the selected objects, above
  Knobmask mouse_down_knob_{kNoKnobs};

  FRIEND_TEST(DocViewTest, KnobsTest);
};

}  // namespace formulate

#endif  // FORMULATE_DOCVIEW_H__
