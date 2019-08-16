// Copyright...

#include <map>

#include "public/fpdf_edit.h"
#include "public/fpdfview.h"

#ifndef PAGE_CACHE_H_
#define PAGE_CACHE_H_

namespace formulate {

class PageCache;
class ScopedObject;
class ScopedPage;

struct Entry {
  explicit Entry(PageCache* cache) : cache_(cache) {}
  virtual ~Entry() = 0;
  void Increment() { count_++; }
  void Decrement() { count_--; }
  int count_{0};
  PageCache* cache_;
};

struct ObjectEntry : public Entry {
  ObjectEntry(PageCache* cache, FPDF_PAGEOBJECT obj)
      : Entry(cache), obj_(obj) {}
  ~ObjectEntry();
  FPDF_PAGEOBJECT obj_;
};

struct PageEntry : public Entry {
  PageEntry(PageCache* cache, FPDF_PAGE page)
      : Entry(cache), page_(page) {}
  ~PageEntry() { FPDF_ClosePage(page_); }
  ScopedObject GetObject(int index);
  void GenerateContentStreams();
  FPDF_PAGE page_;
  std::map<int, ObjectEntry*> objs_;
  bool dirty_{false};
};

class ScopedObject {
 public:
  ScopedObject(ObjectEntry* entry)
      : entry_(entry) { entry->Increment(); }
  ~ScopedObject() { entry_->Decrement(); }
  FPDF_PAGEOBJECT get() const { return entry_->obj_; }
  operator bool() const { return get() != nullptr; }
 private:
  ObjectEntry* entry_;
};

class ScopedPage {
 public:
  ScopedPage(PageEntry* entry)
      : entry_(entry) { entry->Increment(); }
  ~ScopedPage() { entry_->Decrement(); }
  FPDF_PAGE get() const { return entry_->page_; }
  operator bool() const { return get() != nullptr; }
  ScopedObject GetObject(int index) { return entry_->GetObject(index); }
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
