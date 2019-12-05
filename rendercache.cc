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

// This function is a bit of a hack in that it needs the destination
// scale to match the cached image's scale. This function computes
// which bytes need to be copied over and just memcopies them over.
// The cached images pixels, while the same scale, may not align w/
// the destination pixels, but we ignore that and just memcpy them.
// The goal is to be FAST.
void RenderCacheEntry::Draw(SkCanvas* canvas, SkRect rect) {
  if (bitmap_.width() == 0 || bitmap_.height() == 0) {
    fprintf(stderr, "Bitmap is empty\n");
    return;
  }
  if (!Rect().contains(rect)) {
    fprintf(stderr, "can't fulfull draw request\n");
    return;
  }
  SkRect dst_rect = canvas->getTotalMatrix().mapRect(rect);
  SkIRect dst_irect = SkIRect::MakeLTRB(Round(dst_rect.left()),
                                        Round(dst_rect.top()),
                                        Round(dst_rect.right()),
                                        Round(dst_rect.bottom()));
  SkRect src_rect = rect;
  ScaleRect(&src_rect, scale_);
  src_rect.offset(-origin_.x() * scale_, -origin_.y() * scale_);
  SkIRect src_irect = SkIRect::MakeLTRB(Round(src_rect.left()),
                                        Round(src_rect.top()),
                                        Round(src_rect.right()),
                                        Round(src_rect.bottom()));
  int min_width = std::min(src_irect.width(), dst_irect.width());
  src_irect.fRight = src_irect.fLeft + min_width;
  dst_irect.fRight = dst_irect.fLeft + min_width;
  int min_height = std::min(src_irect.height(), dst_irect.height());
  src_irect.fBottom = src_irect.fTop + min_height;
  dst_irect.fBottom = dst_irect.fTop + min_height;

  SkImageInfo dst_info;
  size_t dst_rowbytes;
  char* dst_pixels = static_cast<char*>(
      canvas->accessTopLayerPixels(&dst_info, &dst_rowbytes, nullptr));
  if (!dst_pixels) {
    fprintf(stderr, "can't access dst raw pixels\n");
    return;
  }
  char* dst_end = dst_pixels + dst_rowbytes * dst_info.height();

  if (dst_irect.left() < 0) {
    int offset = -dst_irect.left();
    dst_irect.fLeft += offset;
    src_irect.fLeft += offset;
  }
  if (dst_irect.right() > dst_info.width()) {
    int offset = dst_irect.right() - dst_info.width();
    dst_irect.fRight -= offset;
    src_irect.fRight -= offset;
  }
  if (dst_irect.top() < 0) {
    int offset = -dst_irect.top();
    dst_irect.fTop += offset;
    src_irect.fTop += offset;
  }
  if (dst_irect.bottom() > dst_info.height()) {
    int offset = dst_irect.bottom() - dst_info.height();
    dst_irect.fBottom -= offset;
    src_irect.fBottom -= offset;
  }
  if (dst_irect.isEmpty()) {
    fprintf(stderr, "nothing to draw\n");
    return;
  }
  // char* orig_dst_pixels = dst_pixels;
  dst_pixels += dst_irect.left() * 4 + dst_irect.top() * dst_rowbytes;
  char* src_pixels = reinterpret_cast<char*>(
      bitmap_.getAddr32(src_irect.left(), src_irect.top()));
  size_t src_rowbytes = bitmap_.rowBytes();

  RenderCacheEntry* next = next_;
  int width_bytes = dst_irect.width() * 4;
  for (int i = dst_irect.top(); i < dst_irect.bottom(); i++) {
    if (dst_pixels + width_bytes > dst_end) {
      fprintf(stderr, "about to overrun dst_bytes! i = %d\n", i);
      return;
    }
    memcpy(dst_pixels, src_pixels, width_bytes);
    dst_pixels += dst_rowbytes;
    src_pixels += src_rowbytes;
  }
  if (next != next_) {
    fprintf(stderr, "NEXT CORRUPTED!\n");
  }
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
    it = new RenderCacheEntry(pageno, scale);
  }
  if (!it->Contains(rect)) {
    SkRect render_rect = EnlargeRect(it->Rect(), rect,
                                     renderer_->PageSize(pageno),
                                     scale);
    // Render
    SkBitmap bitmap;
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
      canvas.scale(scale, scale);
      canvas.translate(-render_rect.left(), -render_rect.top());
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

void RenderCache::InvalidateAll() {
  while (head_)
    Remove(head_, nullptr);
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
        Remove(it->next_, it);
      }
      break;
    }
  }
}

}  // namespace formulate
