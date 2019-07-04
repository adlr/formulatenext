// Copyright...

#include "rendercache.h"

namespace formulate {

RenderCacheEntry::~RenderCacheEntry() {
  page = -1;
  if (next_)
    fprintf(stderr, "Deleting RenderCacheEntry with valid next_!\n");
}

void RenderCacheEntry::Draw(SkCanvas* canvas, SkRect rect) {
  fprintf(stderr, "RenderCacheEntry draw page\n");
}

size_t RenderCacheEntry::Size() const {
  return sizeof(*this) + bitmap_.computeByteSize();
}

void RenderCache::DrawPage(SkCanvas* canvas, SkRect rect,
                           int pageno, bool fast) {
  float scale = TODO;
  for (each_entry) {
    if (entry.PartialMatch()) {
      render();
    }
    if (entry.FullMatch()) {
      return;
    }
  }
  // if we get here, delete pages w/ partial match and rerender
  
}

void RenderCache::Invalidate(int pageno, SkRect rect) {
  RenderCacheEntry* prev = nullptr;
  for (RenderCacheEntry* it = head_; it; ) {
    if (it->page() != pageno) {
      it = it->next;
      continue;
    }
    if (prev) {
      prev->next_ = it->next_;
    }
    RenderCacheEntry* temp = it->next_;
    it->next_ = nullptr;
    delete it;
    it = temp;
  }
}

}  // namespace formulate
