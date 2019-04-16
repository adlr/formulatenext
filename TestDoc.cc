#include <emscripten.h>

#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkFont.h"
#include "SkPaint.h"

#include "docview.h"

SkBitmap* bitmap_;
formulate::DocView* view_;

extern "C" {

EMSCRIPTEN_KEEPALIVE
void* TestDraw(int width, int height, float scale) {
  if (bitmap_) {
    if (bitmap_->width() != width ||
	bitmap_->height() != height) {
      delete bitmap_;
      bitmap_ = nullptr;
    }
  }
  if (!bitmap_) {
    bitmap_ = new SkBitmap();
    bitmap_->setInfo(SkImageInfo::Make(width, height,
				      kRGBA_8888_SkColorType,
				      kUnpremul_SkAlphaType));
    if (!bitmap_->tryAllocPixels()) {
      delete bitmap_;
      bitmap_ = nullptr;
      return nullptr;
    }
  }
  if (!view_) {
    view_ = new formulate::DocView(3);
  }

  SkCanvas offscreen(*bitmap_);
  // SkPaint paint;
  // SkFont font;
  // font.setSize(40);
  // font.setSubpixel(true);
  // offscreen.drawString("Hello world", 80, 80, font, paint);
  view_->SetScale(scale);
  view_->Draw(&offscreen);
  return bitmap_->getPixels();
}

}  // extern "C"
