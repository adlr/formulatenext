// Copyright...

#include <gtest/gtest.h>

#include "pdfdoc.h"

int main(int argc, char**argv) {
  formulate::InitPDFium();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
