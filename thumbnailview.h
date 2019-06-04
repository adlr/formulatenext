// Copyright

#ifndef THUMBNAILVIEW_H_
#define THUMBNAILVIEW_H_

#include "SkCanvas.h"

#include "pdfdoc.h"
#include "view.h"

namespace formulate {

class ThumbnailEventHandler {
 public:
  virtual void ScrollToPage(int page) {};
};

class ThumbnailView : public View, public PDFDocEventHandler {
 public:
  explicit ThumbnailView(PDFDoc* doc)
      : doc_(doc) { doc_->AddEventHandler(this); }
  virtual const char* Name() const { return "ThumbnailView"; }
  void SetEventHandler(ThumbnailEventHandler* handler) {
    handler_ = handler;
  }
  void Draw(SkCanvas* canvas, SkRect rect);

  // When the user changes the width. We must update our height.
  void SetWidth(float width);
  int PageForPoint(SkPoint pt) const;

  View* MouseDown(MouseInputEvent ev) { return nullptr; }
  void MouseDrag(MouseInputEvent ev) {}
  void MouseUp(MouseInputEvent ev) {}

  void OnClick(MouseInputEvent ev);

  void SetNeedsDisplayForPage(int page) {
    // TODO(adlr): optimize
    SetNeedsDisplay();
  }

  // Handle page selection
  void SelectPage(int page);  // think: normal click
  void TogglePageSelected(int page);  // think ctrl-click
  void SelectToPage(int page);  // think: shift-click
  bool PageIsSelected(int page) const { return selected_pages_[page]; }

  // PDFDocEventHandler methods
  void PagesChanged();
  void NeedsDisplayInRect(int page, SkRect rect);

 private:
  PDFDoc* doc_;
  ThumbnailEventHandler* handler_{nullptr};
  std::vector<bool> selected_pages_;
  int last_selected_page_{-1};
};

}  // namespace formulate

#endif  // THUMBNAILVIEW_H_
