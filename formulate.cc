#include <emscripten.h>
#include <stddef.h>
#include <stdio.h>

#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_dataavail.h"
#include "public/fpdf_edit.h"
#include "public/fpdfview.h"
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

class Doc {
public:
  Doc(const char* bytes, size_t len)
    : bytes_(bytes), len_(len),
      loader_({bytes_, len_}),
      file_access_({len, TestLoader::GetBlock, &loader_}),
      file_avail_({1, IsDataAvail}),
      hints_({1, AddSegment}),
      pdf_avail_(FPDFAvail_Create(&file_avail_, &file_access_)) {
    // TODO: support for Linearized PDFs
    doc_.reset(FPDF_LoadCustomDocument(&file_access_, nullptr));
    if (!doc_) {
      PrintLastError();
      return;
    }
    if (!FPDF_DocumentHasValidCrossReferenceTable(doc_.get()))
      fprintf(stderr, "Document has invalid cross reference table\n");

    (void)FPDF_GetDocPermissions(doc_.get());
  }

  unsigned char* TestRender(int width, int height,
			    float ma, float mb, float mc, float md,
			    float me, float mf,
			    float sl, float st, float sr, float sb) {
    if (!doc_)
      return nullptr;
    // Make a new bitmap, filled with white
    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 0);
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xffe0e0e0);
    // Render
    FPDF_PAGE page = FPDF_LoadPage(doc_.get(), 0);
    FS_MATRIX matrix = { ma, mb, mc, md, me, mf };
    FS_RECTF clipping = { sl, st, sr, sb };
    FPDF_RenderPageBitmapWithMatrix(bitmap, page, &matrix, &clipping, 0);
    FPDF_ClosePage(page);
    // Copy to output buffer:
    unsigned char* ret =
      static_cast<unsigned char*>(malloc(width * height * 4));
    // src in BGRx. dest in RGBx
    int stride = FPDFBitmap_GetStride(bitmap);
    const unsigned char* buf =
      static_cast<unsigned char*>(FPDFBitmap_GetBuffer(bitmap));
    for (int h = 0; h < height; h++) {
      const unsigned char* src = &buf[h * stride];
      unsigned char* dest = &ret[h * width * 4];
      for (int w = 0; w < width; w++) {
	dest[w * 4    ] = src[w * 4 + 2]; // R
	dest[w * 4 + 1] = src[w * 4 + 1]; // G
	dest[w * 4 + 2] = src[w * 4    ]; // B
	dest[w * 4 + 3] = 0xff;  // alpha, just in case
      }
    }
    FPDFBitmap_Destroy(bitmap);
    return ret;
  }			    

  unsigned char* TestRenderBitmap(int width, int height,
                                  int start_x,
                                  int start_y,
                                  int size_x,
                                  int size_y,
                                  int rotate) {
    if (!doc_)
      return nullptr;
    // Make a new bitmap, filled with white
    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 0);
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xffe0e0e0);
    // Render
    FPDF_PAGE page = FPDF_LoadPage(doc_.get(), 0);
    FPDF_RenderPageBitmap(bitmap, page, start_x, start_y, size_x, size_y,
                          rotate, 0);
    FPDF_ClosePage(page);
    // Copy to output buffer:
    unsigned char* ret =
      static_cast<unsigned char*>(malloc(width * height * 4));
    // src in BGRx. dest in RGBx
    int stride = FPDFBitmap_GetStride(bitmap);
    const unsigned char* buf =
      static_cast<unsigned char*>(FPDFBitmap_GetBuffer(bitmap));
    for (int h = 0; h < height; h++) {
      const unsigned char* src = &buf[h * stride];
      unsigned char* dest = &ret[h * width * 4];
      for (int w = 0; w < width; w++) {
	dest[w * 4    ] = src[w * 4 + 2]; // R
	dest[w * 4 + 1] = src[w * 4 + 1]; // G
	dest[w * 4 + 2] = src[w * 4    ]; // B
	dest[w * 4 + 3] = 0xff;  // alpha, just in case
      }
    }
    FPDFBitmap_Destroy(bitmap);
    return ret;
  }

  // Returns true on success
  bool Render(void* out_buffer, int width, int height,
	      int pageno, float docx, float docy, float docw, float doch) {
    if (!doc_)
      return false;
    ScopedFPDFPage page(FPDF_LoadPage(doc_.get(), pageno));
    if (!page) {
      printf("couldn't get page\n");
      return false;
    }
    ScopedFPDFBitmap bitmap(FPDFBitmap_CreateEx(width, height, FPDFBitmap_BGRA,
						out_buffer, width * 4));
    if (!bitmap) {
      printf("couldn't get bitmap\n");
      return false;
    }
    int alpha = FPDFPage_HasTransparency(page.get()) ? 1 : 0;
    FPDF_DWORD fill_color = alpha ? 0x00000000 : 0xFFFFFFFF;
    FPDFBitmap_FillRect(bitmap.get(), 0, 0, width, height, fill_color);
    FPDF_RenderPageBitmap(bitmap.get(), page.get(), docx, 0, width, height, 0,
			  FPDF_ANNOT);
    return true;
  }
private:
  static FPDF_BOOL IsDataAvail(FX_FILEAVAIL* avail, size_t offset, size_t size) {
    return true;
  }
  static void AddSegment(FX_DOWNLOADHINTS* hints, size_t offset, size_t size) {}

  const char* bytes_;
  size_t len_;
  TestLoader loader_;
  FPDF_FILEACCESS file_access_;
  FX_FILEAVAIL file_avail_;
  FX_DOWNLOADHINTS hints_;
  ScopedFPDFAvail pdf_avail_;
  ScopedFPDFDocument doc_;
};

static const char* doc_bytes_;
static Doc* mydoc_;

extern "C" {

EMSCRIPTEN_KEEPALIVE
int Init(const char* doc_bytes, size_t len) {
  doc_bytes_ = doc_bytes;
  FPDF_LIBRARY_CONFIG config;
  config.version = 2;
  config.m_pUserFontPaths = nullptr;
  config.m_pIsolate = nullptr;
  config.m_v8EmbedderSlot = 0;
  FPDF_InitLibraryWithConfig(&config);
  mydoc_ = new Doc(doc_bytes, len);
  return 22;
}

EMSCRIPTEN_KEEPALIVE
void* AllocateBuffer(int width, int height) {
  return malloc(width * height * 4);
}

EMSCRIPTEN_KEEPALIVE
void FreeBuffer(void* ptr) {
  free(ptr);
}

EMSCRIPTEN_KEEPALIVE
void* OpenDoc(const char* pdf_bytes, size_t len) {
  return new Doc(pdf_bytes, len);
}

EMSCRIPTEN_KEEPALIVE
void FreeDoc(void* doc) {
  Doc* mydoc = static_cast<Doc*>(doc);
  delete mydoc;
}

EMSCRIPTEN_KEEPALIVE
unsigned char* Render(int width, int height,
		      float ma, float mb, float mc, float md,
		      float me, float mf,
		      float sl, float st, float sr, float sb) {
  if (!mydoc_) {
    return nullptr;
  }
  return mydoc_->TestRender(width, height,
			    ma, mb, mc, md, me, mf, sl, st, sr, sb);
}

EMSCRIPTEN_KEEPALIVE
unsigned char* RenderBitmap(int width, int height,
                            int start_x,
                            int start_y,
                            int size_x,
                            int size_y,
                            int rotate) {
  if (!mydoc_) {
    return nullptr;
  }
  return mydoc_->TestRenderBitmap(width, height, start_x, start_y,
                                  size_x, size_y, rotate);
}

EMSCRIPTEN_KEEPALIVE
int FormulateRenderPage(void* doc, void* out_buffer, int width, int height,
			int page, float docx, float docy, float docw, float doch) {
  Doc* mydoc = static_cast<Doc*>(doc);
  return mydoc->Render(out_buffer, width, height, page, docx, docy, docw, doch);
}

}  // extern "C"
