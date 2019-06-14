// Copyright...

#include <gtest/gtest.h>

#include "undo_manager.h"

namespace formulate {

class Client {
 public:
  explicit Client(UndoManager* undo) : undo_(undo) {}
  void Inc() {
    val_++;
    undo_->PushUndoOp([this] () { Dec(); });
  }
  void Dec() {
    val_--;
    undo_->PushUndoOp([this] () { Inc(); });
  }
  void Double() {
    val_ *= 2;
    undo_->PushUndoOp([this] () { Halve(); });
  }
  void Halve() {
    bool odd = val_ % 2;
    val_ /= 2;
    undo_->StartGroup();
    if (odd) {
      undo_->PushUndoOp([this] () { Inc(); });
    }
    undo_->PushUndoOp([this] () { Double(); });
    undo_->EndGroup();
  }
  int val_{0};
  UndoManager* undo_;
};

TEST(UndoManagerTest, UndoRedoTest) {
  UndoManager undo_manager;
  Client client(&undo_manager);
  EXPECT_EQ(0, client.val_);
  client.Inc();
  EXPECT_EQ(1, client.val_);
  EXPECT_EQ(1, undo_manager.undo_ops_.size());
  EXPECT_EQ(0, undo_manager.redo_ops_.size());
  client.Inc();
  EXPECT_EQ(2, client.val_);
  EXPECT_EQ(2, undo_manager.undo_ops_.size());
  EXPECT_EQ(0, undo_manager.redo_ops_.size());
  undo_manager.PerformUndo();
  EXPECT_EQ(1, client.val_);
  EXPECT_EQ(1, undo_manager.undo_ops_.size());
  EXPECT_EQ(1, undo_manager.redo_ops_.size());
  undo_manager.PerformRedo();
  EXPECT_EQ(2, client.val_);
  EXPECT_EQ(2, undo_manager.undo_ops_.size());
  EXPECT_EQ(0, undo_manager.redo_ops_.size());
  undo_manager.PerformUndo();
  EXPECT_EQ(1, client.val_);
  EXPECT_EQ(1, undo_manager.undo_ops_.size());
  EXPECT_EQ(1, undo_manager.redo_ops_.size());
  undo_manager.PerformUndo();
  EXPECT_EQ(0, client.val_);
  EXPECT_EQ(0, undo_manager.undo_ops_.size());
  EXPECT_EQ(2, undo_manager.redo_ops_.size());
  client.Inc();
  EXPECT_EQ(1, client.val_);
  EXPECT_EQ(1, undo_manager.undo_ops_.size());
  EXPECT_EQ(0, undo_manager.redo_ops_.size());
}

TEST(UndoManagerTest, GroupTest) {
  UndoManager undo_manager;
  Client client(&undo_manager);
  client.val_ = 7;
  client.Halve();
  EXPECT_EQ(3, client.val_);
  EXPECT_EQ(1, undo_manager.undo_ops_.size());
  EXPECT_EQ(0, undo_manager.redo_ops_.size());
  EXPECT_EQ(0, undo_manager.group_ops_.size());
  undo_manager.PerformUndo();
  EXPECT_EQ(7, client.val_);
  EXPECT_EQ(0, undo_manager.undo_ops_.size());
  EXPECT_EQ(1, undo_manager.redo_ops_.size());
  EXPECT_EQ(0, undo_manager.group_ops_.size());
  undo_manager.PerformRedo();
  EXPECT_EQ(3, client.val_);
  EXPECT_EQ(1, undo_manager.undo_ops_.size());
  EXPECT_EQ(0, undo_manager.redo_ops_.size());
  EXPECT_EQ(0, undo_manager.group_ops_.size());
  {
    ScopedUndoManagerGroup undo_group(&undo_manager);
    client.Halve();
    EXPECT_EQ(1, client.val_);
    EXPECT_EQ(1, undo_manager.undo_ops_.size());
    EXPECT_EQ(0, undo_manager.redo_ops_.size());
    EXPECT_EQ(2, undo_manager.group_ops_.size());
  }
  EXPECT_EQ(2, undo_manager.undo_ops_.size());
  EXPECT_EQ(0, undo_manager.redo_ops_.size());
  EXPECT_EQ(0, undo_manager.group_ops_.size());
}

}  // namespace formulate
