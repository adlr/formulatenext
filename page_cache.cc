// Copyright...

#include "page_cache.h"

#include <stdio.h>

namespace formulate {

ObjectEntry::~ObjectEntry() {
  // FPDFPageObj_Destroy(obj_);
  if (count_) {
    fprintf(stderr, "Destroying ObjectEntry with opened ScopedObjects. bad!\n");
  }
}

ScopedObject PageEntry::GetObject(int index) {
  auto it = objs_.find(index);
  if (it != objs_.end()) {
    // Cache hit!
    return ScopedObject(it->second);
  }
  ObjectEntry* entry = new ObjectEntry(cache_, FPDFPage_GetObject(page_, index));
  objs_[index] = entry;
  return ScopedObject(entry);
}

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
}

ScopedPage PageCache::OpenPage(int pageno) {
  auto it = pages_.find(pageno);
  if (it != pages_.end()) {
    // cache hit
    return ScopedPage(it->second);
  }
  PageEntry* entry = new PageEntry(this, FPDF_LoadPage(doc_, pageno));
  pages_[pageno] = entry;
  return ScopedPage(entry);
}

}  // namespace formulate
