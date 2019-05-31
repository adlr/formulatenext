// Copyright...

#ifndef FORMULATE_BRIDGE_H_
#define FORMULATE_BRIDGE_H_

#include "scrollview.h"
#include "view.h"

namespace formulate {

void bridge_setSize(ScrollView* view, float width, float height,
                    float xpos, float ypos);
void bridge_UpdateUndoRedoUI(bool undo_enabled, bool redo_unabled);
void bridge_setToolboxState(bool enabled, int tool);
void bridge_startComposingText(SkPoint docpoint, View* view, float zoom);
void bridge_stopComposingText();

// Draw a bezier right on the doc canvas using points in child's view space.
// |bezier| is array of 4 SkPoints, with the following meaning:
// [draw start, ctrl point 1, ctrl point 2, end point].
// |line_width| is in units of child's view space
void bridge_drawBezier(View* child, SkPoint* bezier, float line_width);

}  // namespace formulate

#endif  // FORMULATE_BRIDGE_H_
