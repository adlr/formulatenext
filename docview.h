// Copyright

#include "SkCanvas.h"

namespace formulate {

class DocView {
 public:
  DocView(int pages)
    : pages_(pages) {}
  int Width() const;
  int Height() const;
  void Draw(SkCanvas* ctx);
  void SetScale(float scale) { scale_ = scale; }
 private:
  int pages_;
  float zoom_{1};  // user zoom in/out
  float scale_{1};  // for high-DPI
};

}  // namespace formulate
