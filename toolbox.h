// Copyright...

#ifndef TOOLBOX_H_
#define TOOLBOX_H_

#include "formulate_bridge.h"

namespace formulate {

class Toolbox {
 public:
  enum Tool {
    kArrow_Tool,
    kText_Tool,
    kFreehand_Tool
  };
  Tool current_tool() const { return current_tool_; }
  void set_current_tool(Tool tool) {
    current_tool_ = tool;
  }
  void UpdateUI() const {
    bridge_setToolboxState(true, current_tool_);
  }

 private:
  Tool current_tool_;
};

}  // namespace formulate

#endif  // TOOLBOX_H_
