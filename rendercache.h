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
  bool Contains(SkRect rect) const {
    return Rect().contains(rect);
  }
  void Draw(SkCanvas* canvas, SkRect rect);
  void PageIn(SkBitmap* bitmap, SkPoint origin) {
    origin_ = origin;
    bitmap_ = std::move(*bitmap);
  }
  size_t Size() const;  // Returns approx size of this and bitmap_ in bytes
  int page() const { return page_; }
  float scale() const { return scale_; }
  SkRect Rect() const {
    return SkRect::MakeXYWH(origin_.x(), origin_.y(),
                            bitmap_.width() / scale_,
                            bitmap_.height() / scale_);
  }
  RenderCacheEntry* next_{nullptr};

 private:
  int page_{-1};
  float scale_{0};
  SkPoint origin_;
  SkBitmap bitmap_;
};

class PDFRenderer {
 public:
  virtual SkSize PageSize(int pageno) const = 0;
  virtual void RenderPage(SkCanvas* canvas, SkRect rect, int pageno) const = 0;
};

class RenderCache {
 public:
  ~RenderCache();
  void DrawPage(SkCanvas* canvas, SkRect rect, int pageno, bool fast);
  void Invalidate(int pageno, SkRect rect);
  void SetRenderer(PDFRenderer* renderer) { renderer_ = renderer; }
  void SetMaxSize(size_t bytes) { max_bytes_ = bytes; }

 private:
  // Insert |entry| at head of linked list. It must not be in the list already.
  void InsertAtHead(RenderCacheEntry* entry);
  // Remove |entry| from list. If entry is not head, you must pass the |prev|
  // entry.
  void Remove(RenderCacheEntry* entry, RenderCacheEntry* prev);
  void FreeUpSpace();

  // Will store up to max_bytes_ worth of entries + 1.
  size_t max_bytes_{1024 * 1024 * 4};
  RenderCacheEntry* head_{nullptr};
  PDFRenderer* renderer_{nullptr};
};

}  // namespace formulate

#endif  // RENDERCACHE_H_
