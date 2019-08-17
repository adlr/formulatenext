// Copyright...

#include "page_cache.h"

#include <stdio.h>

namespace formulate {

void PageEntry::GenerateContentStreams() {
  if (!dirty_)
    return;
  FPDFPage_GenerateContent(page_);
  dirty_ = false;
}

void PageCache::GenerateContentStreams() {
  for (auto it : pages_) {
    PageEntry* entry = it.second;
    if (entry->count_) {
      fprintf(stderr, "Page %d has clients while generating content\n",
              it.first);
    }
    entry->GenerateContentStreams();
  }
}

void PageCache::RemoveAllObjects() {
  GenerateContentStreams();
  pages_.clear();
}

ScopedPage PageCache::OpenPage(int pageno) {
  auto it = pages_.find(pageno);
  if (it != pages_.end()) {
    // cache hit
    return ScopedPage(it->second);
  }
  PageEntry* entry = new PageEntry(FPDF_LoadPage(doc_, pageno));
  pages_[pageno] = entry;
  return ScopedPage(entry);
}

}  // namespace formulate
