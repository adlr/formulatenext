// Copyright...

#include "page_cache.h"

#include <stdio.h>

namespace formulate {

PageEntry::PageEntry(AnnotationDelegate* delegate, FPDF_PAGE page)
    : page_(page), annotations_(AnnotationsFromPage(delegate, page_)) {}

void PageEntry::GenerateContentStreams(FPDF_DOCUMENT doc) {
  bool dirty = dirty_;
  if (!dirty) {
    for (auto& annot : annotations_) {
      dirty = annot->dirty();
      if (dirty)
        break;
    }
  }
  if (!dirty)
    return;

  // Store annotations into the page, generate the page stream, then
  // remove them again.
  int orig_obj_cnt = FPDFPage_CountObjects(page_);
  for (auto& annot : annotations_) {
    annot->Flush(doc, page_);
  }
  FPDFPage_GenerateContent(page_);
  while (true) {
    int obj_cnt = FPDFPage_CountObjects(page_);
    if (obj_cnt <= orig_obj_cnt)
      break;
    FPDF_PAGEOBJECT pageobj = FPDFPage_GetObject(page_, obj_cnt - 1);
    if (!pageobj) {
      fprintf(stderr, "Unable to get page obj\n");
      return;
    }
    if (!FPDFPage_RemoveObject(page_, pageobj)) {
      fprintf(stderr, "Unable to remove object\n");
      return;
    }
  }
  dirty_ = false;
}

void PageCache::GenerateContentStreams() {
  for (auto it : pages_) {
    PageEntry* entry = it.second;
    if (entry->count_) {
      fprintf(stderr, "Page %d has clients while generating content\n",
              it.first);
    }
    entry->GenerateContentStreams(doc_);
  }
}

void PageCache::RemoveAllObjects() {
  GenerateContentStreams();
  pages_.clear();
}

ScopedPage PageCache::OpenPage(AnnotationDelegate* delegate, int pageno) {
  auto it = pages_.find(pageno);
  if (it != pages_.end()) {
    // cache hit
    return ScopedPage(it->second);
  }
  PageEntry* entry = new PageEntry(delegate, FPDF_LoadPage(doc_, pageno));
  pages_[pageno] = entry;
  return ScopedPage(entry);
}

}  // namespace formulate
