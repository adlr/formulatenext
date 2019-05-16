// Copyright...

#ifndef UNDO_MANAGER_H_
#define UNDO_MANAGER_H_

#include <deque>
#include <functional>

namespace formulate {

class UndoManager {
 public:
  UndoManager() {}
  void PushUndoOp(const std::function<void ()>& op);
  void PerformUndo();
  void PerformRedo();
  void PerformImpl(std::deque<std::function<void ()>>* ops, bool* in_progress);
  void UpdateUI() const;

 private:
  bool undo_in_progress_{false};
  bool redo_in_progress_{false};
  std::deque<std::function<void ()>> undo_ops_;
  std::deque<std::function<void ()>> redo_ops_;
};

}  // namespace formulate

#endif  // UNDO_MANAGER_H_
