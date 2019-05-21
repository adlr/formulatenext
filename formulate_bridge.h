// Copyright...

#ifndef FORMULATE_BRIDGE_H_
#define FORMULATE_BRIDGE_H_

namespace formulate {

void bridge_UpdateUndoRedoUI(bool undo_enabled, bool redo_unabled);
void bridge_setToolboxState(bool enabled, int tool);
void bridge_startComposingText(float xpos, float ypos, float zoom);
void bridge_stopComposingText();

}  // namespace formulate

#endif  // FORMULATE_BRIDGE_H_
