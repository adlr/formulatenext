#include <emscripten.h>

#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkFont.h"
#include "SkPaint.h"

#include "docview.h"
#include "scrollview.h"

using namespace formulate;

ScrollView* scroll_view_;
DocView* doc_view_;

extern "C" {

EMSCRIPTEN_KEEPALIVE
void SetZoom(float zoom) {
  doc_view_->SetZoom(zoom);
  scroll_view_->RepositionChild();
  scroll_view_->DoDraw();
}

EMSCRIPTEN_KEEPALIVE
void SetScale(float scale) {
  scroll_view_->SetScale(scale);
}

EMSCRIPTEN_KEEPALIVE
void SetSize(float width, float height) {
  scroll_view_->SetSize(SkSize::Make(width, height));
  scroll_view_->DoDraw();
}

EMSCRIPTEN_KEEPALIVE
void SetScrollOrigin(float xpos, float ypos) {
  scroll_view_->SetOrigin(SkPoint::Make(xpos, ypos));
  scroll_view_->DoDraw();
}

EMSCRIPTEN_KEEPALIVE
bool Init() {
  doc_view_ = new DocView();
  scroll_view_ = new ScrollView(doc_view_);
  return true;
}

EMSCRIPTEN_KEEPALIVE
void SetFileSize(size_t length) {
  doc_view_->doc_.SetLength(length);
}

EMSCRIPTEN_KEEPALIVE
void AppendFileBytes(char* bytes, size_t length) {
  doc_view_->doc_.AppendBytes(bytes, length);
}

EMSCRIPTEN_KEEPALIVE
void FinishFileLoad() {
  doc_view_->doc_.FinishLoad();
  doc_view_->RecomputePageSizes();
}

}  // extern "C"
