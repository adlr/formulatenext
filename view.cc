// Copyright

#include "view.h"

namespace formulate {

SkRect ScaleRect(SkRect rect, float scale) {
  rect.fLeft *= scale;
  rect.fTop *= scale;
  rect.fRight *= scale;
  rect.fBottom *= scale;
  return rect;
}

}  // namespace formulate
