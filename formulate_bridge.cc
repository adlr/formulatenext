#include <emscripten.h>

#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkFont.h"
#include "SkPaint.h"

#include "docview.h"
#include "rootview.h"
#include "scrollview.h"
#include "thumbnailview.h"

using namespace formulate;

namespace {
int kIDMain = 0;
int kIDThumb = 1;

RootView* root_views_[2];
ScrollView* scroll_views_[2];
View* leaf_views_[2];
DocView* doc_view_;
ThumbnailView* thumb_view_;
}  // namespace {}

extern "C" {

EMSCRIPTEN_KEEPALIVE
void SetZoom(float zoom) {
  ScopedRedraw redraw(root_views_[kIDMain]);
  SkPoint pt = scroll_views_[kIDMain]->ChildVisibleCenter();
  int page = 1;
  SkPoint pagept;
  doc_view_->ViewPointToPageAndPoint(pt, &page, &pagept);
  doc_view_->SetNeedsDisplay();
  doc_view_->SetZoom(zoom);
  pt = doc_view_->PagePointToViewPoint(page, pagept);
  scroll_views_[kIDMain]->CenterOnChildPoint(pt);
  doc_view_->SetNeedsDisplay();
}

EMSCRIPTEN_KEEPALIVE
void SetScaleAndSize(int id, float scale, float width, float height) {
  if (id != kIDMain && id != kIDThumb) {
    fprintf(stderr, "Invalid ID\n");
    return;
  }
  ScopedRedraw redraw(root_views_[id]);
  root_views_[id]->SetSize(SkSize::Make(scale * width, scale * height));
  if (id == kIDThumb) {
    thumb_view_->SetWidth(width);
  }
  scroll_views_[id]->SetScale(scale);
  scroll_views_[id]->SetSize(SkSize::Make(width, height));
  root_views_[id]->SetNeedsDisplay();
}

EMSCRIPTEN_KEEPALIVE
void SetScrollOrigin(int id, float xpos, float ypos) {
  if (id != kIDMain && id != kIDThumb) {
    fprintf(stderr, "Invalid ID\n");
    return;
  }
  ScopedRedraw redraw(root_views_[id]);
  leaf_views_[id]->SetNeedsDisplay();
  leaf_views_[id]->SetOrigin(SkPoint::Make(-xpos, -ypos));
  scroll_views_[id]->RepositionChild();
  leaf_views_[id]->SetNeedsDisplay();
}

EMSCRIPTEN_KEEPALIVE
bool Init() {
  leaf_views_[0] = doc_view_ = new DocView();
  leaf_views_[1] = thumb_view_ = new ThumbnailView(&doc_view_->doc_);
  for (int i = 0; i < 2; i++) {
    scroll_views_[i] = new ScrollView();
    scroll_views_[i]->AddChild(leaf_views_[i]);
    root_views_[i] = new RootView(i);
    root_views_[i]->AddChild(scroll_views_[i]);
  }
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
  ScopedRedraw redraw(root_views_[kIDMain]);
  doc_view_->doc_.FinishLoad();
  doc_view_->RecomputePageSizes();
  scroll_views_[kIDMain]->RepositionChild();
  doc_view_->SetNeedsDisplay();
  doc_view_->toolbox_.UpdateUI();
}

EMSCRIPTEN_KEEPALIVE
bool MouseEvent(int id, float xpos, float ypos, int type, int modifiers) {
  if (id != kIDMain && id != kIDThumb) {
    fprintf(stderr, "Invalid ID\n");
    return false;
  }
  ScopedRedraw redraw(root_views_[id]);
  MouseInputEvent::Type ty = static_cast<MouseInputEvent::Type>(type);
  MouseInputEvent ev(SkPoint::Make(xpos, ypos), ty, 1, modifiers);
  switch (ty) {
    case MouseInputEvent::DOWN:
      return root_views_[id]->MouseDown(ev) != nullptr;
      break;
    case MouseInputEvent::DRAG:
      root_views_[id]->MouseDrag(ev);
      return false;
      break;
    case MouseInputEvent::UP:
      root_views_[id]->MouseUp(ev);
      return false;
      break;
    case MouseInputEvent::MOVE:
      // TODO(adlr): support hover events
      break;
  }
  return false;
}

EMSCRIPTEN_KEEPALIVE
void DownloadFile() {
  doc_view_->doc_.DownloadDoc();
}

EMSCRIPTEN_KEEPALIVE
void UndoRedoClicked(bool undo) {
  ScopedRedraw redraw_main(root_views_[kIDMain]);
  ScopedRedraw redraw_thumb(root_views_[kIDThumb]);
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

void bridge_setSize(ScrollView* view, float width, float height,
                    float xpos, float ypos) {
  int id = -1;
  if (view == scroll_views_[kIDMain]) {
    id = kIDMain;
  } else if (view == scroll_views_[kIDThumb]) {
    id = kIDThumb;
  } else {
    fprintf(stderr, "bridge_SetSize called w/ invalid view\n");
    return;
  }
  EM_ASM_({
      bridge_setSize($0, $1, $2, $3, $4);
    }, id, width, height, xpos, ypos);
}

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

void bridge_startComposingText(SkPoint docpoint, View* view, float zoom) {
  SkPoint pt = root_views_[kIDMain]->ConvertPointFromChild(view, docpoint);
  EM_ASM_({
      bridge_startComposingText($0, $1, $2);
    }, pt.x(), pt.y(), zoom);
}

void bridge_stopComposingText() {
  EM_ASM({
      bridge_stopComposingText();
    });
}

void bridge_drawBezier(View* child, SkPoint* bezier, float line_width) {
  SkPoint start = root_views_[kIDMain]->ConvertPointFromChild(child, bezier[0]);
  SkPoint ctrl1 = root_views_[kIDMain]->ConvertPointFromChild(child, bezier[1]);
  SkPoint ctrl2 = root_views_[kIDMain]->ConvertPointFromChild(child, bezier[2]);
  SkPoint stop = root_views_[kIDMain]->ConvertPointFromChild(child, bezier[3]);
  SkSize width = root_views_[kIDMain]->ConvertSizeFromChild(
      child, {line_width, 0});
  EM_ASM_({
      bridge_drawBezier($0, $1, $2, $3, $4, $5, $6, $7, $8);
    }, start.x(), start.y(), ctrl1.x(), ctrl1.y(),
    ctrl2.x(), ctrl2.y(), stop.x(), stop.y(), width.width());
}

}  // namespace formulate
