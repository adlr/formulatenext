// Copyright

#ifndef FORMULATE_DOCVIEW_H__
#define FORMULATE_DOCVIEW_H__

#include <vector>

#include "SkCanvas.h"

#include "pdfdoc.h"
#include "view.h"

namespace formulate {

class DocView : public View {
 public:
  DocView() {}
  float Width() const;
  float Height() const;
  void Draw(SkCanvas* canvas, SkRect rect);
  void SetZoom(float zoom) { zoom_ = zoom; }

  void RecomputePageSizes();
  float PageWidth(size_t page) const { return page_sizes_[page].width(); }
  float PageHeight(size_t page) const { return page_sizes_[page].height(); }
  float MaxPageWidth() const { return max_page_width_; }

  PDFDoc doc_;
 private:
  std::vector<SkSize> page_sizes_;  // in PDF points
  float max_page_width_{0};  // in PDF points
  float zoom_{1};  // user zoom in/out
};

}  // namespace formulate

#endif  // FORMULATE_DOCVIEW_H__
