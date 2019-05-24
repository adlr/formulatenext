// Copyright...

#ifndef FORMULATE_PDFDOC_H__
#define FORMULATE_PDFDOC_H__

#include <vector>

#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_dataavail.h"
#include "public/fpdf_edit.h"
#include "public/fpdfview.h"
#include "SkCanvas.h"
#include "SkSize.h"
#include "third_party/base/span.h"

#include "undo_manager.h"

namespace {

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

class PDFDoc {
 public:
  PDFDoc() {}
  ~PDFDoc() {}

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

  // Returns modified rect of the page
  SkRect PlaceText(int pageno, SkPoint pagept, const std::string& ascii);

  // Calls into JS to do the save
  void DownloadDoc() const;
  UndoManager undo_manager_;
 private:
  bool valid_{false};
  std::vector<char> bytes_;
  std::unique_ptr<TestLoader> loader_;
  FPDF_FILEACCESS file_access_;
  ScopedFPDFDocument doc_;
};

}  // namespace formulate

#endif  // FORMULATE_PDFDOC_H__
