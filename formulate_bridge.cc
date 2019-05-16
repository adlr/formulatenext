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
  SkPoint pt = scroll_view_->ChildVisibleCenter();
  int page = 1;
  SkPoint pagept;
  doc_view_->ViewPointToPageAndPoint(pt, &page, &pagept);
  // fprintf(stderr, "\nsc center: %f %f page: %d, pt: %f %f\n",
  //         pt.x(), pt.y(), page, pagept.x(), pagept.y());
  // fprintf(stderr, "set zoom to %f\n", zoom);
  doc_view_->SetZoom(zoom);
  pt = doc_view_->PagePointToViewPoint(page, pagept);
  // fprintf(stderr, "new sc center: %f %f\n", pt.x(), pt.y());
  scroll_view_->CenterOnChildPoint(pt);
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
  scroll_view_->RepositionChild();
  scroll_view_->DoDraw();
  doc_view_->toolbox_.UpdateUI();
}

EMSCRIPTEN_KEEPALIVE
void MouseDown(float xpos, float ypos) {
  scroll_view_->MouseDown(SkPoint::Make(xpos, ypos));
}

EMSCRIPTEN_KEEPALIVE
void MouseDrag(float xpos, float ypos) {
  scroll_view_->MouseDrag(SkPoint::Make(xpos, ypos));
}

EMSCRIPTEN_KEEPALIVE
void MouseUp(float xpos, float ypos) {
  scroll_view_->MouseUp(SkPoint::Make(xpos, ypos));
  scroll_view_->DoDraw();
}

EMSCRIPTEN_KEEPALIVE
void DownloadFile() {
  doc_view_->doc_.DownloadDoc();
}

EMSCRIPTEN_KEEPALIVE
void UndoRedoClicked(bool undo) {
  if (!doc_view_)
    return;
  if (undo) {
    doc_view_->doc_.undo_manager_.PerformUndo();
  } else {
    doc_view_->doc_.undo_manager_.PerformRedo();
  }
}

EMSCRIPTEN_KEEPALIVE
void ToolbarClicked(int tool) {
  if (!doc_view_)
    return;
  doc_view_->toolbox_.set_current_tool(static_cast<Toolbox::Tool>(tool));
}

}  // extern "C"

namespace formulate {

void bridge_UpdateUndoRedoUI(bool undo_enabled, bool redo_unabled) {
  EM_ASM_({
      bridge_undoRedoEnable($0, $1);
    }, undo_enabled, redo_unabled);
}

void bridge_setToolboxState(bool enabled, int tool) {
  EM_ASM_({
      bridge_setToolboxState($0, $1);
    }, enabled, tool);
}

}  // namespace formulate
