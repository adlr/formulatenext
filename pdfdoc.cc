// Copyright...

#include "pdfdoc.h"

#include <emscripten.h>
#include <stdio.h>
#include <wchar.h>

#include "public/fpdf_save.h"

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

void PDFDoc::FinishLoad() {
  {
    FPDF_LIBRARY_CONFIG config;
    config.version = 2;
    config.m_pUserFontPaths = nullptr;
    config.m_pIsolate = nullptr;
    config.m_v8EmbedderSlot = 0;
    FPDF_InitLibraryWithConfig(&config);
  }

  loader_.reset(new TestLoader({&bytes_[0], bytes_.size()}));
  file_access_ = {bytes_.size(), TestLoader::GetBlock, loader_.get()};
  // TODO: support for Linearized PDFs
  doc_.reset(FPDF_LoadCustomDocument(&file_access_, nullptr));
  if (!doc_) {
    PrintLastError();
    return;
  }
  if (!FPDF_DocumentHasValidCrossReferenceTable(doc_.get()))
    fprintf(stderr, "Document has invalid cross reference table\n");

  (void)FPDF_GetDocPermissions(doc_.get());
  valid_ = true;
}

int PDFDoc::Pages() const {
  if (!valid_) return 0;
  return FPDF_GetPageCount(doc_.get());
}

SkSize PDFDoc::PageSize(int page) const {
  SkSize ret({0, 0});
  if (!valid_) return ret;
  double width = 0.0;
  double height = 0.0;
  if (!FPDF_GetPageSizeByIndex(doc_.get(), page, &width, &height)) {
    fprintf(stderr, "FPDF_GetPageSizeByIndex error\n");
  }
  return SkSize::Make(width, height);
}

void DbgMatrix(const char* str, const SkMatrix& mat) {
  fprintf(stderr, "%s: %f %f %f %f %f %f %f %f %f\n", str,
          mat[0], mat[1], mat[2],
          mat[3], mat[4], mat[5],
          mat[6], mat[7], mat[8]);
}

void DbgRect(const char* str, const SkRect& rect) {
  fprintf(stderr, "%s: L: %f T: %f R: %f B: %f (W: %f H: %f)\n", str,
          rect.fLeft, rect.fTop, rect.fRight, rect.fBottom,
          rect.width(), rect.height());
}

void PDFDoc::DrawPage(SkCanvas* canvas, SkRect rect, int pageno) const {
  // In this method we try to find the backing pixels in 'canvas'
  // and feed them directly to the PDF renderer. We assume there
  // are only scale and translate operations applied to the
  // original canvas.

  // We need to map the transform on 'canvas' to the pdfium equivalent, and
  // map 'rect' to the clip argument. This will be impossible if the transform
  // has some rotations/skew, but in practice that shouldn't happen.
  const SkMatrix& mat = canvas->getTotalMatrix();
  SkMatrix inverse;
  if (!mat.invert(&inverse)) {
    fprintf(stderr, "PDFDoc::DrawPage: can't invert matrix\n");
    return;
  }
  SkImageInfo image_info;
  size_t row_bytes;
  void* pixels = canvas->accessTopLayerPixels(&image_info, &row_bytes);
  if (!pixels) {
    fprintf(stderr, "Can't access raw pixels\n");
    return;
  }
  FS_MATRIX pdfmatrix = {mat.getScaleX(),
			 mat.getSkewX(),
			 mat.getSkewY(),
			 mat.getScaleY(),
			 mat.getTranslateX(),
			 mat.getTranslateY()};
  SkRect clip;  // Luckily, FS_RECTF and SkRect are the same
  if (!mat.mapRect(&clip, rect)) {
    fprintf(stderr, "Weird matrix transform: rect doesn't map to rect\n");
    return;
  }
  ScopedFPDFBitmap
    bitmap(FPDFBitmap_CreateEx(image_info.width(),
			       image_info.height(),
			       FPDFBitmap_BGRx,
			       pixels, static_cast<int>(row_bytes)));
  if (!bitmap) {
    fprintf(stderr, "failed to create PDFbitmap\n");
    return;
  }
  // Render the page
  ScopedFPDFPage page(FPDF_LoadPage(doc_.get(), pageno));
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return;
  }
  FPDF_RenderPageBitmapWithMatrix(bitmap.get(), page.get(), &pdfmatrix,
				  reinterpret_cast<FS_RECTF*>(&clip),
				  FPDF_REVERSE_BYTE_ORDER);
  // Render doesn't return anything? I guess it always 'works' heh
}

std::string StrConv(const std::wstring& wstr) {
  std::string ret;
  for (wchar_t w : wstr) {
    ret.push_back(w & 0xff);
    ret.push_back((w >> 8) & 0xff);
  }
  ret.push_back(0);
  ret.push_back(0);
  return ret;
}

void PDFDoc::ModifyPage(int pageno, SkPoint point) {
  fprintf(stderr, "modifying page %d (%f %f)\n", pageno,
          point.x(), point.y());
  ScopedFPDFPage page(FPDF_LoadPage(doc_.get(), pageno));
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return;
  }

  FPDF_PAGEOBJECT textobj =
    FPDFPageObj_NewTextObj(doc_.get(), "Helvetica", 12.0f);
  if (!textobj) {
    fprintf(stderr, "Unable to allocate text obj\n");
    return;
  }
  std::string message = StrConv(L"Hello, world!");
  FPDF_WIDESTRING pdf_str = reinterpret_cast<FPDF_WIDESTRING>(message.c_str());
  if (!FPDFText_SetText(textobj, pdf_str)) {
    fprintf(stderr, "failed to set text\n");
    return;
  }

  FPDFPageObj_Transform(textobj, 1, 0, 0, 1, point.x(), point.y());
  FPDFPage_InsertObject(page.get(), textobj);
  if (!FPDFPage_GenerateContent(page.get())) {
    fprintf(stderr, "PDFPage_GenerateContent failed\n");
  }
}

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

void PDFDoc::DownloadDoc() const {
  FileSaver fs;
  if (!FPDF_SaveAsCopy(doc_.get(), &fs, FPDF_REMOVE_SECURITY)) {
    fprintf(stderr, "FPDF_SaveAsCopy failed\n");
    return;
  }
  EM_ASM_({
      bridge_downloadBytes($0, $1);
    }, &fs.data_[0], fs.data_.size());
}

}  // namespace formulate
