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

  double page_width, page_height;
  if (!FPDF_GetPageSizeByIndex(doc_.get(), pageno, &page_width, &page_height)) {
    fprintf(stderr, "can't get page size\n");
    return;
  }

  // SkPaint paint;
  // paint.setStyle(SkPaint::kStroke_Style);
  // paint.setColor(0xff808080);  // opaque grey
  // paint.setStrokeWidth(1);

  // int objcnt = FPDFPage_CountObjects(page.get());
  // for (int i = 0; i < objcnt; i++) {
  //   FPDF_PAGEOBJECT pageobj = FPDFPage_GetObject(page.get(), i);
  //   if (!pageobj) {
  //     fprintf(stderr, "Unable to get pageobj!\n");
  //     return;
  //   }
  //   SkRect bounds;
  //   if (!FPDFPageObj_GetBounds(pageobj,
  //                              &bounds.fLeft,
  //                              &bounds.fBottom,
  //                              &bounds.fRight,
  //                              &bounds.fTop)) {
  //     fprintf(stderr, "Unable to get bounds\n");
  //     return;
  //   }
  //   bounds.fTop = page_height - bounds.fTop;
  //   bounds.fBottom = page_height - bounds.fBottom;
  //   canvas->drawRect(bounds, paint);
  // }
  // fprintf(stderr, "Drawing found %d objects\n", objcnt);
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

std::string StrConv(const std::string& ascii) {
  // Convert low-order ascii string to UTF-16LE
  std::string ret;
  for (char c : ascii) {
    if (c < 0)
      return "";
    ret.push_back(c);
    ret.push_back(0);
  }
  ret.push_back(0);
  ret.push_back(0);
  return ret;
}

void PDFDoc::DeleteObjUnderPoint(int pageno, SkPoint point) {
  int i = -1;
  {
    ScopedFPDFPage page(FPDF_LoadPage(doc_.get(), pageno));
    if (!page) {
      fprintf(stderr, "failed to load PDFPage\n");
      return;
    }
    int objcount = FPDFPage_CountObjects(page.get());
    for (i = objcount - 1; i >= 0; i--) {
      FPDF_PAGEOBJECT pageobj = FPDFPage_GetObject(page.get(), i);
      if (!pageobj) {
        fprintf(stderr, "Unable to get pageobj!\n");
        return;
      }
      SkRect bounds;
      if (!FPDFPageObj_GetBounds(pageobj,
                                 &bounds.fLeft,
                                 &bounds.fTop,
                                 &bounds.fRight,
                                 &bounds.fBottom)) {
        fprintf(stderr, "Unable to get bounds\n");
        return;
      }
      fprintf(stderr, "Bounds: %f %f %f %f, pt %f %f type %d\n",
              bounds.fLeft, bounds.fTop, bounds.fRight, bounds.fBottom,
              point.x(), point.y(),
              FPDFPageObj_GetType(pageobj));
      if (bounds.contains(point.x(), point.y())) {
        // Delete this object!
        break;
      }
    }
    if (i < 0) {
      fprintf(stderr, "Couldn't find an object under given point\n");
      return;
    }
  }
  DeleteObject(pageno, i);
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
    FPDFPageObj_NewTextObj(doc_.get(), "Times-Roman", 12.0f);
  if (!textobj) {
    fprintf(stderr, "Unable to allocate text obj\n");
    return;
  }
  std::string message = StrConv(L"Hello, world! 5%pz&*$gdkt\nline2");
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

void PDFDoc::DeleteObject(int pageno, int index) {
  {
    fprintf(stderr, "DeleteObject(%d, %d) called\n", pageno, index);
    ScopedFPDFPage page(FPDF_LoadPage(doc_.get(), pageno));
    if (!page) {
      fprintf(stderr, "failed to load PDFPage\n");
      return;
    }
    FPDF_PAGEOBJECT pageobj = FPDFPage_GetObject(page.get(), index);
    if (!pageobj) {
      fprintf(stderr, "Unable to get page obj\n");
      return;
    }
    fprintf(stderr, "Deleting of type %d\n",
            FPDFPageObj_GetType(pageobj));
    int beforeocnt = FPDFPage_CountObjects(page.get());
    fprintf(stderr, "about to call FPDFPage_RemoveObject\n");
    if (!FPDFPage_RemoveObject(page.get(), pageobj)) {
      fprintf(stderr, "Unable to remove object\n");
      return;
    }
    fprintf(stderr, "called FPDFPage_RemoveObject\n");
    int afterocnt = FPDFPage_CountObjects(page.get());
    if (!FPDFPage_GenerateContent(page.get())) {
      fprintf(stderr, "PDFPage_GenerateContent failed\n");
    }
    int afterocnt2 = FPDFPage_CountObjects(page.get());
    fprintf(stderr, "Before: %d AFter: %d aftergen: %d\n", beforeocnt, afterocnt,
            afterocnt2);
    // Generate undo op.
    // TODO(adlr): This is leaking pageojb if the UndoManager removes
    // this undo op without performing it
    undo_manager_.PushUndoOp(
        [this, pageno, index, pageobj] () {
          InsertObject(pageno, index, pageobj);
        });
  }
  ScopedFPDFPage page(FPDF_LoadPage(doc_.get(), pageno));
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return;
  }
  int ocnt = FPDFPage_CountObjects(page.get());
  fprintf(stderr, "after close/open: %d objects\n", ocnt);
}

void PDFDoc::InsertObject(int pageno, int index, FPDF_PAGEOBJECT pageobj) {
  fprintf(stderr, "InsertObject(%d, %d) called\n", pageno, index);
  ScopedFPDFPage page(FPDF_LoadPage(doc_.get(), pageno));
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return;
  }
  // TODO(adlr): use index. Currently the API is not there for it
  fprintf(stderr, "calling FPDFPage_InsertObject\n");
  fprintf(stderr, "before we have %d objs\n", FPDFPage_CountObjects(page.get()));
  FPDFPage_InsertObject(page.get(), pageobj);
  fprintf(stderr, "after we have %d objs\n", FPDFPage_CountObjects(page.get()));

  double a, b, c, d, e, f;
  int rc = FPDFText_GetMatrix(pageobj, &a, &b, &c, &d, &e, &f);
  fprintf(stderr, "get matrix(%d) = [%f %f %f %f %f %f]\n", rc,
          a, b, c, d, e, f);

  fprintf(stderr, "Going to count objects\n");
  index = FPDFPage_CountObjects(page.get()) - 1;
  fprintf(stderr, "going to generate content\n");
  if (!FPDFPage_GenerateContent(page.get())) {
    fprintf(stderr, "PDFPage_GenerateContent failed\n");
  }
  fprintf(stderr, "making undo op\n");
  undo_manager_.PushUndoOp(
      [this, pageno, index] () {
        DeleteObject(pageno, index);
      });
}

void PDFDoc::PlaceText(int pageno, SkPoint pagept, const std::string& ascii) {
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
  std::string message = StrConv(ascii);
  FPDF_WIDESTRING pdf_str = reinterpret_cast<FPDF_WIDESTRING>(message.c_str());
  if (!FPDFText_SetText(textobj, pdf_str)) {
    fprintf(stderr, "failed to set text\n");
    return;
  }

  FPDFPageObj_Transform(textobj, 1, 0, 0, 1, pagept.x(), pagept.y());
  FPDFPage_InsertObject(page.get(), textobj);
  double a, b, c, d, e, f;
  int rc = FPDFText_GetMatrix(textobj, &a, &b, &c, &d, &e, &f);
  fprintf(stderr, "get matrix(%d) = [%f %f %f %f %f %f]\n", rc,
          a, b, c, d, e, f);
  if (!FPDFPage_GenerateContent(page.get())) {
    fprintf(stderr, "PDFPage_GenerateContent failed\n");
  }
  // Handle Undo
  int index = FPDFPage_CountObjects(page.get()) - 1;
  undo_manager_.PushUndoOp(
      [this, pageno, index] () {
        DeleteObject(pageno, index);
      });
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
