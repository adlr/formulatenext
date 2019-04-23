// Copyright...

#include "view.h"

namespace formulate {

class CenteringView : public View {
 public:
  CenteringView(View* child)
    : child_(child) {}
  double Width()
 private:
  View* child_;
  SkSize outer_size_;
}

}  // namespace formulate
