// Copyright

#include "SkCanvas.h"

namespace formulate {

class View {
 public:
  View() {}
  virtual ~View() {}
  virtual double Width() = 0;
  virtual double Height() = 0;
  virtual void Draw(SkCanvas* ctx) = 0;
};

}  // namespace formulate
