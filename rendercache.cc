// Copyright...

#include "rendercache.h"

namespace formulate {

namespace {

void ScaleRect(SkRect* rect, float scale) {
  rect->fLeft *= scale;
  rect->fTop *= scale;
  rect->fRight *= scale;
  rect->fBottom *= scale;
}

SkIRect PageToBitmapRect(SkRect rect, float scale) {
  ScaleRect(&rect, scale);
  SkIRect irect;
  rect.roundOut(&irect);
  return irect;
}

SkRect MakeDiscreet(SkRect rect, float scale) {
  SkIRect irect = PageToBitmapRect(rect, scale);
  SkRect ret(SkRect::Make(irect));
  ScaleRect(&ret, 1 / scale);
  return ret;
}

int Round(float fl) { return static_cast<int>(fl + 0.5); }

}  // namespace {}

RenderCacheEntry::RenderCacheEntry(int page, float scale)
    : page_(page), scale_(scale), origin_(SkPoint::Make(0, 0)) {}

RenderCacheEntry::~RenderCacheEntry() {
  page_ = -1;
  if (next_)
    fprintf(stderr, "Deleting RenderCacheEntry with valid next_!\n");
}

void RenderCacheEntry::Draw(SkCanvas* canvas, SkRect rect) {
  SkIRect src = PageToBitmapRect(rect, scale_);
  SkRect dst(SkRect::Make(src));
  ScaleRect(&dst, 1 / scale_);
  src.offset(-Round(origin_.x() * scale_), -Round(origin_.y() * scale_));
  canvas->drawBitmapRect(bitmap_, src, dst, nullptr);
}

size_t RenderCacheEntry::Size() const {
  return sizeof(*this) + bitmap_.computeByteSize();
}

namespace {

SkRect EnlargeRect(SkRect current, SkRect requested,
                   SkSize pagesize, float scale) {
  requested.outset(300 / scale, 300 / scale);
  if (!requested.intersect(SkRect::MakeSize(pagesize))) {
    fprintf(stderr, "enlarged render rect doesn't intersect page size. weird\n");
  }
  requested = MakeDiscreet(requested, scale);
  return requested;
}

}  // namespace {}

RenderCache::~RenderCache() {
  while (head_) {
    RenderCacheEntry* temp = head_;
    Remove(head_, nullptr);
    delete temp;
  }
}

void RenderCache::DrawPage(SkCanvas* canvas, SkRect rect,
                           int pageno, bool fast) {
  // Compute scale
  float scale = 1.0;
  {
    const SkMatrix& mat = canvas->getTotalMatrix();
    if (mat.getScaleX() != mat.getScaleY()) {
      fprintf(stderr, "x and y scales differ! %f %f\n",
              mat.getScaleX(), mat.getScaleY());
      
    }
    scale = mat.getScaleX();
    fprintf(stderr, "Render page %d with scale %f\n", pageno, scale);
  }
  RenderCacheEntry* prev = nullptr;
  RenderCacheEntry* it;
  for (it = head_; it; ) {
    if (it->page() == pageno && it->scale() == scale) {
      Remove(it, prev);  // it is removed from list. We will add it back later.
      break;
    }
    prev = it;
    it = it->next_;
  }
  if (!it) {
    // not found. allocate a new one.
    fprintf(stderr, "miss\n");
    it = new RenderCacheEntry(pageno, scale);
  }
  if (!it->Contains(rect)) {
    fprintf(stderr, "need to rerender\n");
    SkRect render_rect = EnlargeRect(it->Rect(), rect,
                                     renderer_->PageSize(pageno),
                                     scale);
    fprintf(stderr, "Rect enlarged %f %f %f %f -> %f %f %f %f\n",
            rect.left(), rect.top(), rect.right(), rect.bottom(),
            render_rect.left(), render_rect.top(),
            render_rect.right(), render_rect.bottom());
    // Render
    SkBitmap bitmap;
    fprintf(stderr, "render width %f, %d\n", render_rect.width() * scale,
            Round(render_rect.width() * scale));
    bitmap.setInfo(SkImageInfo::Make(Round(render_rect.width() * scale),
                                     Round(render_rect.height() * scale),
                                     kRGBA_8888_SkColorType,
                                     kUnpremul_SkAlphaType));
    if (!bitmap.tryAllocPixels()) {
      fprintf(stderr, "failed to alloc bitmap in rendercache\n");
      return;
    }
    {
      SkCanvas canvas(bitmap);
      canvas.translate(render_rect.left(), render_rect.top());
      canvas.scale(scale, scale);
      // Fill with opaque white
      SkPaint paint;
      paint.setAntiAlias(false);
      paint.setStyle(SkPaint::kFill_Style);
      paint.setColor(0xffffffff);  // opaque white
      paint.setStrokeWidth(0);
      canvas.drawRect(render_rect, paint);
      renderer_->RenderPage(&canvas, render_rect, pageno);
    }
    it->PageIn(&bitmap, SkPoint::Make(render_rect.left(),
                                      render_rect.top()));
    {
      SkRect temp = it->Rect();
      fprintf(stderr, "it now has %f %f %f %f\n",
              temp.left(), temp.top(), temp.right(), temp.bottom());
    }            
    if (!it->Contains(rect)) {
      fprintf(stderr, "Rendering to cache failed!\n");
      return;
    }
  }
  it->Draw(canvas, rect);
  InsertAtHead(it);
  FreeUpSpace();
  return;
}

void RenderCache::Invalidate(int pageno, SkRect rect) {
  RenderCacheEntry* prev = nullptr;
  for (RenderCacheEntry* it = head_; it; ) {
    if (pageno == it->page()) {
      // For now just delete it all
      Remove(it, prev);
      delete it;
      if (!prev) {
        it = head_;
      } else {
        it = prev->next_;
      }
      continue;
    }
    prev = it;
    it = it->next_;
  }
}

void RenderCache::InsertAtHead(RenderCacheEntry* entry) {
  entry->next_ = head_;
  head_ = entry;
}

void RenderCache::Remove(RenderCacheEntry* entry, RenderCacheEntry* prev) {
  if (prev) {
    prev->next_ = entry->next_;
  } else {
    head_ = entry->next_;
  }
  entry->next_ = nullptr;
}

void RenderCache::FreeUpSpace() {
  size_t bytes_found = 0;
  for (RenderCacheEntry* it = head_; it; it = it->next_) {
    bytes_found += it->Size();
    if (bytes_found > max_bytes_) {
      // Delete all future entries
      while (it->next_) {
        fprintf(stderr, "Cache: Removing page %d (%f) to free space\n",
                it->next_->page(), it->next_->scale());
        Remove(it->next_, it);
      }
      return;
    }
  }
}

}  // namespace formulate
