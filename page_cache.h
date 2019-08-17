// Copyright...

#include <map>

#include "public/fpdf_edit.h"
#include "public/fpdfview.h"

#ifndef PAGE_CACHE_H_
#define PAGE_CACHE_H_

namespace formulate {

class PageCache;
class ScopedPage;

struct PageEntry {
  explicit PageEntry(FPDF_PAGE page) : page_(page) {}
  ~PageEntry() { FPDF_ClosePage(page_); }
  FPDF_PAGEOBJECT GetObject(int index) {
    return FPDFPage_GetObject(page_, index);
  }
  void GenerateContentStreams();
  int count_{0};
  FPDF_PAGE page_{nullptr};
  bool dirty_{false};
};

class ScopedPage {
 public:
  explicit ScopedPage(PageEntry* entry)
      : entry_(entry) { entry_->count_++; }
  ~ScopedPage() { entry_->count_--; }
  FPDF_PAGE get() const { return entry_->page_; }
  operator bool() const { return get() != nullptr; }
  FPDF_PAGEOBJECT GetObject(int index) { return entry_->GetObject(index); }
  void MarkDirty() { entry_->dirty_ = true; }
 private:
  PageEntry* entry_;
};

class PageCache {
 public:
  PageCache() {}
  void SetDoc(FPDF_DOCUMENT doc) { doc_ = doc; }
  ~PageCache() { RemoveAllObjects(); }
  ScopedPage OpenPage(int pageno);
  void GenerateContentStreams();
  void RemoveAllObjects();

 private:
  FPDF_DOCUMENT doc_;
  std::map<int, PageEntry*> pages_;
};

}  // namespace formulate

#endif  // PAGE_CACHE_H_
