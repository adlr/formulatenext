// Copyright...

#include "undo_manager.h"

#include "formulate_bridge.h"

namespace formulate {

void UndoManager::PushUndoOp(const std::function<void ()>& op) {
  if (group_) {
    group_ops_.push_back(op);
  } else if (undo_in_progress_) {
    redo_ops_.push_back(op);
  } else {
    if (!redo_in_progress_)
      redo_ops_.clear();
    undo_ops_.push_back(op);
    while (undo_ops_.size() > 10) {
      undo_ops_.pop_front();
    }
  }
  UpdateUI();
}

void UndoManager::PerformUndo() {
  PerformImpl(&undo_ops_, &undo_in_progress_);
}

void UndoManager::PerformRedo() {
  PerformImpl(&redo_ops_, &redo_in_progress_);
}

void UndoManager::PerformImpl(std::deque<std::function<void ()>>* ops,
                              bool* in_progress) {
  if (ops->empty())
    return;
  *in_progress = true;
  ops->back()();
  ops->pop_back();
  *in_progress = false;
  UpdateUI();
}

void UndoManager::UpdateUI() const {
  bridge_UpdateUndoRedoUI(!undo_ops_.empty(),
                          !redo_ops_.empty());
}

void UndoManager::EndGroup() {
  group_--;
  if (group_)
    return;
  if (group_ops_.empty())
    return;
  if (group_ops_.size() == 1) {
    PushUndoOp(std::move(group_ops_[0]));
    group_ops_.clear();
    return;
  }
  PushUndoOp([this, group_ops_{move(group_ops_)}] () {
      StartGroup();
      for (auto it = group_ops_.rbegin(); it != group_ops_.rend(); ++it) {
        (*it)();
      }
      EndGroup();
   });
}

}  // namespace formulate
