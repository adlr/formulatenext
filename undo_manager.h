// Copyright...

#ifndef UNDO_MANAGER_H_
#define UNDO_MANAGER_H_

#include <deque>
#include <functional>
#include <vector>

#include <gtest/gtest.h>

namespace formulate {

class UndoManager {
 public:
  UndoManager() {}
  void PushUndoOp(const std::function<void ()>& op);
  void PerformUndo();
  void PerformRedo();
  void PerformImpl(std::deque<std::function<void ()>>* ops, bool* in_progress);
  void UpdateUI() const;
  void StartGroup() { group_++; }
  void EndGroup();

 private:
  bool undo_in_progress_{false};
  bool redo_in_progress_{false};
  std::deque<std::function<void ()>> undo_ops_;
  std::deque<std::function<void ()>> redo_ops_;
  int group_{0};
  std::vector<std::function<void ()>> group_ops_;
  FRIEND_TEST(UndoManagerTest, UndoRedoTest);
  FRIEND_TEST(UndoManagerTest, GroupTest);
};

class ScopedUndoManagerGroup {
 public:
  explicit ScopedUndoManagerGroup(UndoManager* undo) : undo_(undo) {
    undo_->StartGroup();
  }
  ~ScopedUndoManagerGroup() { undo_->EndGroup(); }
 private:
  UndoManager* undo_{nullptr};
};

}  // namespace formulate

#endif  // UNDO_MANAGER_H_
