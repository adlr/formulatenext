// Copyright...

namespace formulate {

void PDFDoc::FinshLoad() {
  loader_ = {&bytes_[0], bytes_.size()};
  file_access_ = {bytes_.size(), TestLoader::GetBlock, &loader_};
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
  if (!valid) return 0;
  return FPDF_GetPageCount(doc_.get());
}

SkSize PDFDoc::PageSize(int page) const {
  SkSize ret({0, 0});
  if (!valid) return ret;
  double width = 0.0;
  double height = 0.0;
  if (FPDF_GetPageSizeByIndex(doc_.get(), page, &width, &height)) {
    fprintf(stderr, "FPDF_GetPageSizeByIndex error\n");
  }
  ret = {width, height};
  return ret;
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
  FS_MATRIX pdfmatrix = {matrix.getScaleX(),
			 matrix.getSkewX(),
			 matrix.getSkewY(),
			 matrix.getScaleY(),
			 matrix.getTranslateX(),
			 matrix.getTranslateY()};
  SkRect clip;  // Luckily, FS_RECTF and SkRect are the same
  if (!inverse.mapRect(&clip, rect)) {
    fprintf(stderr, "Weird matrix transform: rect doesn't map to rect\n");
    return;
  }
  ScopedFPDFBitmap
    bitmap(FPDFBitmap_CreateEx(image_info.width,
			       image_info.height,
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
				  static_cast<FS_RECTF*>(&clip),
				  FPDF_REVERSE_BYTE_ORDER);
  // Render doesn't return anything? I guess it always 'works' heh
  return true;
}

void PDFDoc::ModifyPage(int page, SkPoint point) {

}

class FileSaver : public FPDF_FILEWRITE {
 public:
  FileSaver() : version(1), WriteBlock(StaticWriteBlock) {}
  static int StaticWriteBlock(FPDF_FILEWRITE* pthis,
                              const void* data,
                              unsigned long size) {
    FileSaver* fs = static_cast<FileSaver*>(pthis);
    const char* cdata = static_cast<char*>(data);
    fs->data_.insert(fs->data_.end(), cdata, cdata + size);
  }
  std::vector<char> data_;
}

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
