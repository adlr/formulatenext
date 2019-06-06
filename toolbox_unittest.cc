// Copyright...

#include <gtest/gtest.h>

#include "toolbox.h"

using namespace formulate;

TEST(ToolboxTest, GetSetTest) {
  Toolbox toolbox;
  toolbox.set_current_tool(Toolbox::kText_Tool);
  EXPECT_EQ(Toolbox::kText_Tool, toolbox.current_tool());
  toolbox.set_current_tool(Toolbox::kArrow_Tool);
  EXPECT_EQ(Toolbox::kArrow_Tool, toolbox.current_tool());
}
