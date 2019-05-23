// Copyright

#ifndef FORMULATE_DOCVIEW_H__
#define FORMULATE_DOCVIEW_H__

#include <vector>

#include "SkCanvas.h"

#include "pdfdoc.h"
#include "toolbox.h"
#include "view.h"

namespace formulate {

class DocView : public View {
 public:
  DocView() {}
  void Draw(SkCanvas* canvas, SkRect rect);
  void SetZoom(float zoom) { zoom_ = zoom; }

  void RecomputePageSizes();
  float PageWidth(size_t page) const { return page_sizes_[page].width(); }
  float PageHeight(size_t page) const { return page_sizes_[page].height(); }
  float MaxPageWidth() const { return max_page_width_; }

  // Convert point in view coords (within Width() and Height()) to a
  // page and point within that page. The point within that page is in PDF
  // coordinates, so it doesn't take zoom into acct.
  void ViewPointToPageAndPoint(const SkPoint& viewpt,
                               int* out_page,
                               SkPoint* out_pagept) const;

  // Converts a point in PDF coordinates of a given page to view coords.
  // The inverse of ViewPointToPageAndPoint().
  SkPoint PagePointToViewPoint(int page, const SkPoint& pagept) const;

  // void MouseDown(SkPoint pt);
  // void MouseDrag(SkPoint pt);
  // void MouseUp(SkPoint pt);

  PDFDoc doc_;
  Toolbox toolbox_;

  void SetEditingString(const char* str) {
    editing_text_str_ = str;
  }
 private:
  std::vector<SkSize> page_sizes_;  // in PDF points
  float max_page_width_{0};  // in PDF points
  float zoom_{1};  // user zoom in/out
  int editing_text_page_{-1};
  SkPoint editing_text_point_;  // in PDF points
  std::string editing_text_str_;
};

}  // namespace formulate

#endif  // FORMULATE_DOCVIEW_H__
