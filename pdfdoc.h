// Copyright...

#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_dataavail.h"
#include "public/fpdf_edit.h"
#include "public/fpdfview.h"
#include "SkSize.h"
#include "third_party/base/span.h"

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

void PrintLastError() {
  unsigned long err = FPDF_GetLastError();
  fprintf(stderr, "Load pdf docs unsuccessful: ");
  switch (err) {
  case FPDF_ERR_SUCCESS:
    fprintf(stderr, "Success");
    break;
  case FPDF_ERR_UNKNOWN:
    fprintf(stderr, "Unknown error");
    break;
  case FPDF_ERR_FILE:
    fprintf(stderr, "File not found or could not be opened");
    break;
  case FPDF_ERR_FORMAT:
    fprintf(stderr, "File not in PDF format or corrupted");
    break;
  case FPDF_ERR_PASSWORD:
    fprintf(stderr, "Password required or incorrect password");
    break;
  case FPDF_ERR_SECURITY:
    fprintf(stderr, "Unsupported security scheme");
    break;
  case FPDF_ERR_PAGE:
    fprintf(stderr, "Page not found or content error");
    break;
  default:
    fprintf(stderr, "Unknown error %ld", err);
  }
  fprintf(stderr, ".\n");
  return;
}

class PDFDoc {
 public:
  PDFDoc() {}
  ~PDFDoc() {}

  // Load the initial doc from byte buffer
  void SetLength(size_t len) { bytes_.reserve(len); }
  void AppendBytes(char* bytes, size_t len) {
    bytes_.insert(bytes_.end(), bytes, bytes + len);
  }
  void FinshLoad();

  int Pages() const;
  SkSize PageSize(int page) const;
  void DrawPage(SkCanvas* canvas, SkRect rect) const;

  // Test to make a change to a doc
  void ModifyPage(int page, SkPoint point);

  // Calls into JS to do the save
  void DownloadDoc() const;
 private:
  bool valid_{false};
  std::vector<char> bytes_;
  TestLoader loader_;
  FPDF_FILEACCESS file_access_;
  ScopedFPDFDocument doc_;
}

}  // namespace formulate
