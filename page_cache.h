// Copyright...

#ifndef PAGE_CACHE_H_
#define PAGE_CACHE_H_

#include <map>

#include "public/fpdf_edit.h"
#include "public/fpdfview.h"

#include "annotation.h"

namespace formulate {

class PageCache;
class ScopedPage;

// Stores an open FPDF_PAGE in memory and the Annotation objects that
// this software knows how to manipulate. Note that when opening a
// page of a PDF, it is modified to remove the objects that correspond
// to Annotatoin objects. The PageEntry is NOT marked dirty in this
// case, tho, b/c no user-facing change was made. Only when a user
// facing change is made will the page be marked dirty. When
// regenerating the content stream, it will only happen if the page is
// dirty.

struct PageEntry {
  PageEntry(AnnotationDelegate* delegate, FPDF_PAGE page);
  ~PageEntry() { FPDF_ClosePage(page_); page_ = nullptr; }
  FPDF_PAGEOBJECT GetObject(int index) {
    return FPDFPage_GetObject(page_, index);
  }
  void GenerateContentStreams(FPDF_DOCUMENT doc);
  FPDF_PAGE page_{nullptr};
  std::vector<std::unique_ptr<Annotation>> annotations_;
  int count_{0};
  bool dirty_{false};
};

class ScopedPage {
 public:
  explicit ScopedPage(PageEntry* entry)
      : entry_(entry) { entry_->count_++; }
  ~ScopedPage() { entry_->count_--; }
  FPDF_PAGE get() const { return entry_->page_; }
  operator bool() const { return get() != nullptr; }
  // FPDF_PAGEOBJECT GetObject(int index) { return entry_->GetObject(index); }
  size_t AnnotationCount() const { return entry_->annotations_.size(); }
  const std::vector<std::unique_ptr<Annotation>>& Annotations() {
    return entry_->annotations_;
  }
  Annotation* GetAnnotation(size_t index) {
    return entry_->annotations_[index].get(); }
  void PushAnnotation(std::unique_ptr<Annotation> annot) {
    entry_->annotations_.emplace_back(std::move(annot));
  }
  void MarkDirty() { entry_->dirty_ = true; }
 private:
  PageEntry* entry_;
};

class PageCache {
 public:
  PageCache() {}
  void SetDoc(FPDF_DOCUMENT doc) { doc_ = doc; }
  ~PageCache() { RemoveAllObjects(); }
  ScopedPage OpenPage(AnnotationDelegate* delegate, int pageno);
  void GenerateContentStreams();
  void RemoveAllObjects();

 private:
  FPDF_DOCUMENT doc_;
  std::map<int, PageEntry*> pages_;
};

}  // namespace formulate

#endif  // PAGE_CACHE_H_
