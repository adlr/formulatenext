#include <emscripten.h>

#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkFont.h"
#include "SkPaint.h"

#include "docview.h"
#include "scrollview.h"
#include "rootview.h"

using namespace formulate;

RootView* root_view_;
ScrollView* scroll_view_;
DocView* doc_view_;

extern "C" {

EMSCRIPTEN_KEEPALIVE
void SetZoom(float zoom) {
  ScopedRedraw redraw(root_view_);
  SkPoint pt = scroll_view_->ChildVisibleCenter();
  int page = 1;
  SkPoint pagept;
  doc_view_->ViewPointToPageAndPoint(pt, &page, &pagept);
  // fprintf(stderr, "\nsc center: %f %f page: %d, pt: %f %f\n",
  //         pt.x(), pt.y(), page, pagept.x(), pagept.y());
  // fprintf(stderr, "set zoom to %f\n", zoom);
  doc_view_->SetNeedsDisplay();
  doc_view_->SetZoom(zoom);
  pt = doc_view_->PagePointToViewPoint(page, pagept);
  // fprintf(stderr, "new sc center: %f %f\n", pt.x(), pt.y());
  scroll_view_->CenterOnChildPoint(pt);
  doc_view_->SetNeedsDisplay();
}

EMSCRIPTEN_KEEPALIVE
void SetScaleAndSize(float scale, float width, float height) {
  ScopedRedraw redraw(root_view_);
  root_view_->SetSize(SkSize::Make(scale * width, scale * height));
  scroll_view_->SetScale(scale);
  scroll_view_->SetSize(SkSize::Make(width, height));
  root_view_->SetNeedsDisplay();
}

EMSCRIPTEN_KEEPALIVE
void SetScrollOrigin(float xpos, float ypos) {
  ScopedRedraw redraw(root_view_);
  doc_view_->SetNeedsDisplay();
  doc_view_->SetOrigin(SkPoint::Make(-xpos, -ypos));
  scroll_view_->RepositionChild();
  doc_view_->SetNeedsDisplay();
}

EMSCRIPTEN_KEEPALIVE
bool Init() {
  doc_view_ = new DocView();
  scroll_view_ = new ScrollView();
  scroll_view_->AddChild(doc_view_);
  root_view_ = new RootView();
  fprintf(stderr, "set rootview to non-null\n");
  root_view_->AddChild(scroll_view_);
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
  ScopedRedraw redraw(root_view_);
  doc_view_->doc_.FinishLoad();
  doc_view_->RecomputePageSizes();
  scroll_view_->RepositionChild();
  doc_view_->toolbox_.UpdateUI();
}

EMSCRIPTEN_KEEPALIVE
void MouseDown(float xpos, float ypos) {
  ScopedRedraw redraw(root_view_);
  scroll_view_->MouseDown(SkPoint::Make(xpos, ypos));
}

EMSCRIPTEN_KEEPALIVE
void MouseDrag(float xpos, float ypos) {
  ScopedRedraw redraw(root_view_);
  scroll_view_->MouseDrag(SkPoint::Make(xpos, ypos));
}

float hack_xpos;
float hack_ypos;

EMSCRIPTEN_KEEPALIVE
void MouseUp(float xpos, float ypos) {
  ScopedRedraw redraw(root_view_);
  hack_xpos = xpos;
  hack_ypos = ypos;
  scroll_view_->MouseUp(SkPoint::Make(xpos, ypos));
}

EMSCRIPTEN_KEEPALIVE
void DownloadFile() {
  doc_view_->doc_.DownloadDoc();
}

EMSCRIPTEN_KEEPALIVE
void UndoRedoClicked(bool undo) {
  ScopedRedraw redraw(root_view_);
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

EMSCRIPTEN_KEEPALIVE
void UpdateEditText(const char* str) {
  doc_view_->SetEditingString(str);
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

void bridge_startComposingText(float xpos, float ypos, float zoom) {
  EM_ASM_({
      bridge_startComposingText($0, $1);
    }, hack_xpos, hack_ypos);
}

void bridge_stopComposingText() {
  EM_ASM({
      bridge_stopComposingText();
    });
}

}  // namespace formulate
