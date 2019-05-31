// Copyright

#ifndef THUMBNAILVIEW_H_
#define THUMBNAILVIEW_H_

#include "SkCanvas.h"

#include "pdfdoc.h"
#include "view.h"

namespace formulate {

class ThumbnailView : public View, public PDFDocEventHandler {
 public:
  explicit ThumbnailView(PDFDoc* doc)
      : doc_(doc) { doc_->AddEventHandler(this); }
  void Draw(SkCanvas* canvas, SkRect rect);

  // When the user changes the width. We must update our height.
  void SetWidth(float width);

  View* MouseDown(MouseInputEvent ev) { return nullptr; }
  void MouseDrag(MouseInputEvent ev) {}
  void MouseUp(MouseInputEvent ev) {}

  // PDFDocEventHandler methods
  void PagesChanged() {
  }
  void NeedsDisplayInRect(int page, SkRect rect) {
  }
 private:
  PDFDoc* doc_;
};

}  // namespace formulate

#endif  // THUMBNAILVIEW_H_
