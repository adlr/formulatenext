// Copyright

# include "scrollview.h"

namespace formulate {

void Scrollview::Draw(SkCanvas* canvas) {
  canvas->scale(scale_, scale_);
  canvas->translate(origin_.x(), origin_.y());
  child_->Draw(canvas);
}

}  // namespace formulate
