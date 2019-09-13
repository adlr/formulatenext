// Copyright...

#ifndef FORMULATE_PDFDOC_H__
#define FORMULATE_PDFDOC_H__

#include <set>
#include <vector>

#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_dataavail.h"
#include "public/fpdf_edit.h"
#include "public/fpdf_save.h"
#include "public/fpdfview.h"
#include "SkCanvas.h"
#include "SkPath.h"
#include "SkSize.h"
#include "third_party/base/span.h"

#include "page_cache.h"
#include "rendercache.h"
#include "undo_manager.h"

namespace {

class FileSaver : public FPDF_FILEWRITE {
 public:
  FileSaver() {
    version = 1;
    WriteBlock = StaticWriteBlock;
  }
  static int StaticWriteBlock(FPDF_FILEWRITE* pthis,
                              const void* data,
                              unsigned long size) {
    FileSaver* fs = static_cast<FileSaver*>(pthis);
    const char* cdata = static_cast<const char*>(data);
    fs->data_.insert(fs->data_.end(), cdata, cdata + size);
    return 1;  // success
  }
  std::vector<char> data_;
};

class TestLoader {
 public:
  explicit TestLoader(pdfium::span<const char> span)
    : m_Span(span) {}

  static int GetBlock(void* param,
		      unsigned long pos,
		      unsigned char* pBuf,
		      unsigned long size) {
    TestLoader* pLoader = static_cast<TestLoader*>(param);
    if (pos + size < pos || pos + size > pLoader->m_Span.size()) {
      NOTREACHED();
      return 0;
    }

    memcpy(pBuf, &pLoader->m_Span[pos], size);
    return 1;
  }

 private:
  const pdfium::span<const char> m_Span;
};

}  // namespace {}

namespace formulate {

void PrintLastError();

std::string StrToUTF16LE(const std::wstring& wstr);
std::string StrToUTF16LE(const std::string& ascii);
// chars must be an array with even number of bytes. The last two must be
// null, which indicates end of the array.
std::string UTF16LEToStr(const unsigned char* chars);

void InitPDFium();

class PDFDocEventHandler {
 public:
  // Number of pages and/or page sizes changed
  virtual void PagesChanged() {}
  virtual void NeedsDisplayInRect(int page, SkRect rect) {}
  virtual void NeedsDisplayForObj(int page, int index) {}
};

class PDFDoc : PDFRenderer {
 public:
  enum ObjType {
    kUnknown,
    kText,
    kPath,
    kImage,
    kShading,
    kForm
  };

  PDFDoc() {
    render_cache.SetRenderer(this);
  }
  ~PDFDoc() {}
  void AddEventHandler(PDFDocEventHandler* handler) {
    event_handlers_.push_back(handler);
  }

  // Load the initial doc from byte buffer
  void SetLength(size_t len) { bytes_.reserve(len); }
  void AppendBytes(char* bytes, size_t len) {
    bytes_.insert(bytes_.end(), bytes, bytes + len);
  }
  void FinishLoad();

  int Pages() const;
  SkSize PageSize(int page) const;

  // Attemps to render w/ the cache
  void DrawPage(SkCanvas* canvas, SkRect rect, int pageno);

  // Does the actual render
  void RenderPage(SkCanvas* canvas, SkRect rect, int pageno) const;

  // Test to make a change to a doc
  void ModifyPage(int pageno, SkPoint point);
  void DeleteObjUnderPoint(int pageno, SkPoint point);

  // Returns the index of the object under |pt| or -1 if not found.
  // If |native| is true, it will only find an object that's native
  // to this software.
  int ObjectUnderPoint(int pageno, SkPoint pt, bool native) const;

 public:
  SkRect BoundingBoxForObj(int pageno, int index) const;
  ObjType ObjectType(int pageno, int index) const;
  // Returns the body string of a text object as UTF-8. Return empty string
  // on error.
  std::string TextObjValue(int pageno, int index) const;
  // Returns the origin point of the text object (the left baseline point)
  SkPoint TextObjOrigin(int pageno, int index) const;
  int TextObjCaretPosition(int pageno, int objindex, float xpos) const;

  void DeleteObject(int pageno, int index);
  void InsertObject(int pageno, int index, FPDF_PAGEOBJECT pageobj);
  int ObjectsOnPage(int pageno) const;

  void InsertPath(int pageno, SkPoint center, const SkPath& path);

  void PlaceText(int pageno, SkPoint pagept, const std::string& ascii);
  void UpdateText(int pageno, int index, const std::string& ascii,
                  const std::string& orig_value, bool undo);
  void InsertFreehandDrawing(int pageno, const std::vector<SkPoint>& pts);

  void MoveObjects(int pageno, const std::set<int>& objs,
                   float dx, float dy, bool do_move, bool do_undo);
  void SetObjectBounds(int pageno, int objindex, SkRect bounds);

  void DumpAPAtPagePt(int pageno, SkPoint pt);

  // Move the pages in the range [start, end) to index |to|.
  void MovePages(int start, int end, int to);

  // Move many in-order ranges of pages to |to|. Each range in |from| is
  // of the form [start, end).
  void MovePages(const std::vector<std::pair<int, int>>& from, int to);

  // Calls into JS to do the save
  void DownloadDoc() const;

  void AppendPDF(const char* bytes, size_t length);

  void SetCacheMaxSize(size_t bytes) { render_cache.SetMaxSize(bytes); }

  UndoManager undo_manager_;
 private:
  RenderCache render_cache;

  bool valid_{false};
  std::vector<char> bytes_;
  std::unique_ptr<TestLoader> loader_;
  FPDF_FILEACCESS file_access_;
  ScopedFPDFDocument doc_;
  mutable PageCache pagecache_;
  std::vector<PDFDocEventHandler*> event_handlers_;
};

}  // namespace formulate

#endif  // FORMULATE_PDFDOC_H__
