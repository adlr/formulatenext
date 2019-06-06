// Copyright...

#include "formulate_bridge.h"

namespace formulate {

void bridge_setSize(ScrollView* view, float width, float height,
                    float xpos, float ypos) {}
void bridge_UpdateUndoRedoUI(bool undo_enabled, bool redo_unabled) {}
void bridge_setToolboxState(bool enabled, int tool) {}
void bridge_startComposingText(SkPoint docpoint, View* view, float zoom) {}
void bridge_stopComposingText() {}
void bridge_drawBezier(View* child, SkPoint* bezier, float line_width) {}
void bridge_drawPixels(int id, void* ptr, int xpos,
                       int ypos, int width, int height) {}
void bridge_downloadBytes(void* ptr, int len) {}

}  // namespace formulate
