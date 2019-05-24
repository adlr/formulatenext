// Copyright...

#ifndef FORMULATE_BRIDGE_H_
#define FORMULATE_BRIDGE_H_

#include "view.h"

namespace formulate {

void bridge_UpdateUndoRedoUI(bool undo_enabled, bool redo_unabled);
void bridge_setToolboxState(bool enabled, int tool);
void bridge_startComposingText(SkPoint docpoint, View* view, float zoom);
void bridge_stopComposingText();

}  // namespace formulate

#endif  // FORMULATE_BRIDGE_H_
