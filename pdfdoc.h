// Copyright...

#ifndef FORMULATE_PDFDOC_H__
#define FORMULATE_PDFDOC_H__

#include <vector>

#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_dataavail.h"
#include "public/fpdf_edit.h"
#include "public/fpdf_save.h"
#include "public/fpdfview.h"
#include "SkCanvas.h"
#include "SkSize.h"
#include "third_party/base/span.h"

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

void InitPDFium();

class PDFDocEventHandler {
 public:
  // Number of pages and/or page sizes changed
  virtual void PagesChanged() {}
  virtual void NeedsDisplayInRect(int page, SkRect rect) {}
};

class PDFDoc {
 public:
  PDFDoc() {}
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
  void DrawPage(SkCanvas* canvas, SkRect rect, int pageno) const;

  // Test to make a change to a doc
  void ModifyPage(int pageno, SkPoint point);
  void DeleteObjUnderPoint(int pageno, SkPoint point);

  void DeleteObject(int pageno, int index);
  void InsertObject(int pageno, int index, FPDF_PAGEOBJECT pageobj);
  int ObjectsOnPage(int pageno) const;

  void PlaceText(int pageno, SkPoint pagept, const std::string& ascii);
  void InsertFreehandDrawing(int pageno, const std::vector<SkPoint>& pts);

  // Move the pages in the range [start, end) to index |to|.
  void MovePages(int start, int end, int to);

  // Move many in-order ranges of pages to |to|. Each range in |from| is
  // of the form [start, end).
  void MovePages(const std::vector<std::pair<int, int>>& from, int to);

  // Calls into JS to do the save
  void DownloadDoc() const;

  void AppendPDF(const char* bytes, size_t length);

  UndoManager undo_manager_;
 private:
  bool valid_{false};
  std::vector<char> bytes_;
  std::unique_ptr<TestLoader> loader_;
  FPDF_FILEACCESS file_access_;
  ScopedFPDFDocument doc_;
  std::vector<PDFDocEventHandler*> event_handlers_;
};

}  // namespace formulate

#endif  // FORMULATE_PDFDOC_H__
