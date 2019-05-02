// Copyright

#include "SkCanvas.h"

#include "view.h"

namespace formulate {

class DocView : public View {
 public:
  DocView(int pages)
    : pages_(pages) {}
  int Width() const;
  int Height() const;
  void Draw(SkCanvas* canvas);
  void SetZoom(float zoom) { zoom_ = zoom; }
  void SetScale(float scale) { scale_ = scale; }
 private:
  int pages_;
  float zoom_{1};  // user zoom in/out
  float scale_{1};  // for high-DPI
};

}  // namespace formulate

