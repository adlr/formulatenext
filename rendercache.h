// Copyright...

#ifndef RENDERCACHE_H_
#define RENDERCACHE_H_

#include <memory>

#include "SkBitmap.h"
#include "SkCanvas.h"

namespace formulate {

class RenderCacheEntry {
 public:
  RenderCacheEntry(int page, float scale);
  ~RenderCacheEntry();
  bool PartialMatch(SkRect rect) const;
  bool FullMatch(SkRect rect) const;
  void Draw(SkCanvas* canvas, SkRect rect);
  size_t Size() const;  // Returns approx size of this and bitmap_ in bytes
  int page() const { return page_; }
  float scale() const { return scale_; }
  RenderCacheEntry* next_{nullptr};

 private:
  int page_{-1};
  float scale_{0};
  SkPoint origin_;
  SkBitmap bitmap_;
};

class RenderCache {
 public:
  DrawPage(SkCanvas* canvas, SkRect rect, int pageno, bool fast);
  void Invalidate(int pageno, SkRect rect);

 private:
  size_t max_bytes_;
  int pixel_overscan_;  // Additional pixels to render in each direction
  RenderCacheEntry* head_{nullptr};
};

}  // namespace formulate

#endif  // RENDERCACHE_H_
