// Copyright...

#include "pdfdoc.h"

#include <stdio.h>
#include <wchar.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include "public/fpdf_annot.h"
#include "public/fpdf_ppo.h"
#include "public/fpdf_save.h"

#include "formulate_bridge.h"
#include "rich_format.h"

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

void InitPDFium() {
  FPDF_LIBRARY_CONFIG config;
  config.version = 2;
  config.m_pUserFontPaths = nullptr;
  config.m_pIsolate = nullptr;
  config.m_v8EmbedderSlot = 0;
  FPDF_InitLibraryWithConfig(&config);
}

void PDFDoc::FinishLoad() {
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
  pagecache_.SetDoc(doc_.get());
  for (PDFDocEventHandler* handler : event_handlers_)
    handler->PagesChanged();
  DumpPage(0);
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

void PDFDoc::DumpPage(int pageno) const {
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return;
  }
  int objcount = FPDFPage_CountObjects(page.get());
  for (int i = objcount - 1; i >= 0; i--) {
    SkRect bbox = BoundingBoxForObj(pageno, i);
    FPDF_PAGEOBJECT obj = page.GetObject(i);
    fprintf(stderr, "BBox: %f %f %f %f type: %d\n",
            bbox.fLeft, bbox.fTop, bbox.fRight, bbox.fBottom,
            FPDFPageObj_GetType(obj));
  }
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

void PDFDoc::DrawPage(SkCanvas* canvas, SkRect rect, int pageno) {
  render_cache.DrawPage(canvas, rect, pageno, false);
}

void PDFDoc::RenderPage(SkCanvas* canvas, SkRect rect, int pageno) const {
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
  ScopedPage page = const_cast<PageCache&>(pagecache_).OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return;
  }
  FPDF_RenderPageBitmapWithMatrix(bitmap.get(), page.get(), &pdfmatrix,
				  reinterpret_cast<FS_RECTF*>(&clip),
				  FPDF_ANNOT | FPDF_REVERSE_BYTE_ORDER);
  // Render doesn't return anything? I guess it always 'works' heh

  double page_width, page_height;
  if (!FPDF_GetPageSizeByIndex(doc_.get(), pageno, &page_width, &page_height)) {
    fprintf(stderr, "can't get page size\n");
    return;
  }
}

std::string StrToUTF16LE(const std::wstring& wstr) {
  std::string ret;
  for (wchar_t w : wstr) {
    ret.push_back(w & 0xff);
    ret.push_back((w >> 8) & 0xff);
  }
  ret.push_back(0);
  ret.push_back(0);
  return ret;
}

std::string StrToUTF16LE(const std::string& ascii) {
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

std::string UTF16LEToStr(const unsigned char* chars) {
  // Only handles low-order ascii for now
  std::string ret;
  for (int i = 0; chars[i] || chars[i + 1]; i += 2) {
    if (chars[i] > 127 || chars[i + 1]) {
      fprintf(stderr, "Can't handle high-order string value\n");
      return std::string();
    }
    ret.push_back(chars[i]);
  }
  return ret;
}

int PDFDoc::ObjectUnderPoint(int pageno, SkPoint pt, bool native) const {
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return -1;
  }
  int objcount = FPDFPage_CountObjects(page.get());
  for (int i = objcount - 1; i >= 0; i--) {
    SkRect bbox = BoundingBoxForObj(pageno, i);
    // fprintf(stderr, "BBox: %f %f %f %f vs %f %f\n",
    //         bbox.fLeft, bbox.fTop, bbox.fRight, bbox.fBottom,
    //         pt.x(), pt.y());
    if (bbox.contains(pt.x(), pt.y()))
      return i;
  }
  return -1;
}

SkRect PDFDoc::BoundingBoxForObj(int pageno, int index) const {
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return SkRect();
  }
  FPDF_PAGEOBJECT obj = page.GetObject(index);
  float left, bottom, right, top;
  if (!FPDFPageObj_GetBounds(obj, &left, &bottom, &right, &top)) {
    fprintf(stderr, "GetBounds failed\n");
    return SkRect();
  }
  return SkRect::MakeLTRB(left, bottom, right, top);
}

PDFDoc::ObjType PDFDoc::ObjectType(int pageno, int index) const {
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return kUnknown;
  }
  FPDF_PAGEOBJECT obj = page.GetObject(index);
  if (!obj) {
    fprintf(stderr, "Page has no object at index %d\n", index);
    return kUnknown;
  }
  return static_cast<ObjType>(FPDFPageObj_GetType(obj));
}

std::string PDFDoc::TextObjValue(int pageno, int index) const {
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return std::string();
  }
  FPDF_PAGEOBJECT obj = page.GetObject(index);
  if (!obj) {
    fprintf(stderr, "Page has no object at index %d\n", index);
    return std::string();
  }
  if (FPDFPageObj_GetType(obj) != FPDF_PAGEOBJ_TEXT) {
    fprintf(stderr, "Object is not text object\n");
    return std::string();
  }
  // Get length
  unsigned long len = FPDFTextObj_GetText(obj, nullptr, nullptr, 0);
  if (len == 0 || len & 1) {
    fprintf(stderr, "GetText failed\n");
    return std::string();
  }
  unsigned char chars[len];
  if (!FPDFTextObj_GetText(obj, nullptr, chars, len)) {
    fprintf(stderr, "GetText failed when reading string\n");
    return std::string();
  }
  chars[len - 1] = chars[len - 2] = '\0';
  fprintf(stderr, "gettext has a length of %d\n", static_cast<int>(len));
  return UTF16LEToStr(chars);
}

SkPoint PDFDoc::TextObjOrigin(int pageno, int index) const {
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return SkPoint::Make(0, 0);
  }
  FPDF_PAGEOBJECT obj = page.GetObject(index);
  if (!obj) {
    fprintf(stderr, "Page has no object at index %d\n", index);
    return SkPoint::Make(0, 0);
  }
  if (FPDFPageObj_GetType(obj) != FPDF_PAGEOBJ_TEXT) {
    fprintf(stderr, "Object is not text object\n");
    return SkPoint::Make(0, 0);
  }
  double a, b, c, d, e, f;
  if (!FPDFText_GetMatrix(obj, &a, &b, &c, &d, &e, &f)) {
    fprintf(stderr, "FPDFText_GetMatrix failed\n");
    return SkPoint::Make(0, 0);
  }
  fprintf(stderr, "Text has matrix: %f %f %f %f %f %f\n", a, b, c, d, e, f);
  return SkPoint::Make(e, f);
}

int PDFDoc::TextObjCaretPosition(int pageno, int objindex, float xpos) const {
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return -1;
  }
  FPDF_PAGEOBJECT obj = page.GetObject(objindex);
  if (!obj) {
    fprintf(stderr, "Page has no object at index %d\n", objindex);
    return -1;
  }
  if (FPDFPageObj_GetType(obj) != FPDF_PAGEOBJ_TEXT) {
    fprintf(stderr, "Object is not text object\n");
    return -1;
  }
  return FPDFText_GetIndexForOffset(obj, xpos);
}

void PDFDoc::DeleteObject(int pageno, int index) {
  {
    fprintf(stderr, "DeleteObject(%d, %d) called\n", pageno, index);
    ScopedPage page = pagecache_.OpenPage(pageno);
    if (!page) {
      fprintf(stderr, "failed to load PDFPage\n");
      return;
    }
    FPDF_PAGEOBJECT pageobj = page.GetObject(index);
    if (!pageobj) {
      fprintf(stderr, "Unable to get page obj\n");
      return;
    }
    SkRect dirty;
    if (!FPDFPageObj_GetBounds(pageobj, &dirty.fLeft, &dirty.fTop,
                              &dirty.fRight, &dirty.fBottom)) {
      fprintf(stderr, "Failed to get bounds for new text obj\n");
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
    page.MarkDirty();
    int afterocnt2 = FPDFPage_CountObjects(page.get());
    fprintf(stderr, "Before: %d AFter: %d aftergen: %d\n", beforeocnt, afterocnt,
            afterocnt2);
    // Generate undo op.
    // TODO(adlr): This is leaking pageojb if the UndoManager removes
    // this undo op without performing it
    render_cache.Invalidate(pageno, dirty);
    for (PDFDocEventHandler* handler : event_handlers_)
      handler->NeedsDisplayInRect(pageno, dirty);
    undo_manager_.PushUndoOp(
        [this, pageno, index, pageobj] () {
          InsertObject(pageno, index, pageobj);
        });
  }
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return;
  }
  int ocnt = FPDFPage_CountObjects(page.get());
  fprintf(stderr, "after close/open: %d objects\n", ocnt);
}

void PDFDoc::InsertObject(int pageno, int index, FPDF_PAGEOBJECT pageobj) {
  fprintf(stderr, "InsertObject(%d, %d) called\n", pageno, index);
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return;
  }
  // TODO(adlr): use index. Currently the API is not there for it
  fprintf(stderr, "calling FPDFPage_InsertObject\n");
  fprintf(stderr, "before we have %d objs\n", FPDFPage_CountObjects(page.get()));
  FPDFPage_InsertObject(page.get(), pageobj);
  fprintf(stderr, "after we have %d objs\n", FPDFPage_CountObjects(page.get()));

  // double a, b, c, d, e, f;
  // int rc = FPDFText_GetMatrix(pageobj, &a, &b, &c, &d, &e, &f);
  // fprintf(stderr, "get matrix(%d) = [%f %f %f %f %f %f]\n", rc,
  //         a, b, c, d, e, f);

  fprintf(stderr, "Going to count objects\n");
  index = FPDFPage_CountObjects(page.get()) - 1;
  page.MarkDirty();
  fprintf(stderr, "making undo op\n");
  undo_manager_.PushUndoOp(
      [this, pageno, index] () {
        DeleteObject(pageno, index);
      });
  SkRect dirty;
  if (FPDFPageObj_GetBounds(pageobj, &dirty.fLeft, &dirty.fTop,
                            &dirty.fRight, &dirty.fBottom)) {
    render_cache.Invalidate(pageno, dirty);

    for (PDFDocEventHandler* handler : event_handlers_)
      handler->NeedsDisplayInRect(pageno, dirty);
  } else {
    fprintf(stderr, "Failed to get bounds for new obj\n");
  }
}

int PDFDoc::ObjectsOnPage(int pageno) const {
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return 0;
  }
  return FPDFPage_CountObjects(page.get());
}

void PDFDoc::InsertPath(int pageno, SkPoint center, const SkPath& path) {
  ScopedFPDFPageObject obj(FPDFPageObj_CreateNewPath(0, 0));
  SkPath::RawIter it(path);
  float pgheight = PageSize(pageno).height();
  while (true) {
    SkPoint pts[4];
    SkPath::Verb verb = it.next(pts);
    if (verb == SkPath::kDone_Verb)
      break;
    if (verb == SkPath::kMove_Verb) {
      if (!FPDFPath_MoveTo(obj.get(), pts[0].x(), pgheight - pts[0].y())) {
        fprintf(stderr, "moveto failed\n");
        return;
      }
    } else if (verb == SkPath::kLine_Verb) {
      if (!FPDFPath_LineTo(obj.get(), pts[1].x(), pgheight - pts[1].y())) {
        fprintf(stderr, "lineto failed\n");
        return;
      }
    } else if (verb == SkPath::kCubic_Verb) {
      if (!FPDFPath_BezierTo(obj.get(),
                             pts[1].x(), pgheight - pts[1].y(),
                             pts[2].x(), pgheight - pts[2].y(),
                             pts[3].x(), pgheight - pts[3].y())) {
        fprintf(stderr, "bezierto failed\n");
        return;
      }
    }
  }

  if (path.getFillType() != SkPath::kEvenOdd_FillType) {
    fprintf(stderr, "Unknown fill type\n");
    return;
  } else {
    if (!FPDFPath_SetDrawMode(obj.get(), FPDF_FILLMODE_ALTERNATE, false)) {
      fprintf(stderr, "setdrawmode failed\n");
      return;
    }
  }

  SkRect bounds = path.computeTightBounds();
  if ((bounds.right() - bounds.left()) < 0.001) {
    fprintf(stderr, "path is too thin %f %f\n", bounds.right(), bounds.left());
    return;
  }
  // conver to PDF coordinates
  bounds.fTop = pgheight - bounds.fTop;
  bounds.fBottom = pgheight - bounds.fBottom;

  // Set width to 3 inches.
  const float swidth = 72 * 3;
  float scale = swidth / (bounds.right() - bounds.left());
  float sheight = (bounds.top() - bounds.bottom()) * scale;

  bounds.fLeft *= scale;
  bounds.fTop *= scale;
  bounds.fRight *= scale;
  bounds.fBottom *= scale;

  float dx = center.x() - (bounds.left() + swidth / 2);
  float dy = center.y() - (bounds.bottom() + sheight / 2);

  fprintf(stderr, "center: %f %f, ltrb: %f %f %f %f (%f %f %f)\n",
          center.x(), center.y(),
          bounds.left(), bounds.top(), bounds.right(), bounds.bottom(),
          dx, dy, scale);

  FPDFPageObj_Transform(obj.get(), scale, 0, 0, scale, dx, dy);
  InsertObject(pageno, 0, obj.release());
}

namespace {

// extern "C" {
//   extern const unsigned char g_FoxitSansFontData[15025];
// }

// struct SkEmbeddedResource {const uint8_t* data; const size_t size;};
// struct SkEmbeddedHeader {const SkEmbeddedResource* entries; const int count;};
// extern "C" SkEmbeddedHeader const ARIMO_FONT;

void TestShape() {
  return;

  // init freetype
  // FT_Library ftlib;
  // FT_Error err = FT_Init_FreeType(&ftlib);
  // if (err) {
  //   fprintf(stderr, "FT_Init_FreeType failed\n");
  //   return;
  // }

  // load Arimo
  // hb_blob_t* arimo_blob =
  //     hb_blob_create(reinterpret_cast<const char*>(ARIMO_FONT.entries[0].data),
  //                    ARIMO_FONT.entries[0].size,
  //                    HB_MEMORY_MODE_READONLY,
  //                    nullptr, nullptr);
  // hb_blob_make_immutable(arimo_blob);
  // hb_face_t* face = hb_face_create(arimo_blob, 0);
  // hb_blob_destroy(arimo_blob);
  // hb_face_set_index(face, 0);
  //hb_face_set_upem(face, ???);

  // FT_Face ft_face;
  // // err = FT_New_Memory_Face(ftlib, g_FoxitSansFontData,
  // //                          sizeof(g_FoxitSansFontData), 0, &ft_face);
  // err = FT_New_Memory_Face(ftlib, ARIMO_FONT.entries[0].data,
  //                          ARIMO_FONT.entries[0].size, 0, &ft_face);
  // if (err) {
  //   fprintf(stderr, "FT_New_Memory_Face failed\n");
  //   return;
  // }

  // // const hb_tag_t KernTag = HB_TAG('k', 'e', 'r', 'n');
  // // hb_feature_t KerningOn   = { KernTag, 1, 0, std::numeric_limits<unsigned int>::max() };
  
  // fprintf(stderr, "line height: %d\n", (int)ft_face->height);
  // fprintf(stderr, "Font name: [%s] [%s]\n",
  //         ft_face->family_name, ft_face->style_name);
  // FT_Set_Char_Size(ft_face, 0, 1200, 0, 0);

  // {
  //   // get first/last char info
  //   FT_UInt first_gid = 0;
  //   FT_ULong first_char = FT_Get_First_Char(ft_face, &first_gid);
  //   fprintf(stderr, "first gid/char: %u/%lu\n", first_gid, first_char);
  //   int limit = 300;
  //   while (first_gid != 0) {
  //     first_char = FT_Get_Next_Char(ft_face, first_char, &first_gid);
  //     fprintf(stderr, "      git/char: %u/%lu\n", first_gid, first_char);
  //     if (limit-- == 0) {
  //       fprintf(stderr, "breakout!\n");
  //       break;
  //     }
  //   }
  // }

  // hb_font_t* hb_font = hb_ft_font_create(ft_face, nullptr);
  // // hb_font_t* hb_font = hb_font_create(face);

  // hb_font_set_scale(hb_font, 1200, 1200);
  // const char* user_input = "LAVA TVTlMfiM";
  // hb_buffer_t *hb_buffer = hb_buffer_create();
  // hb_buffer_add_utf8(hb_buffer, user_input, -1, 0, -1);
  // hb_buffer_guess_segment_properties(hb_buffer);

  // hb_buffer_set_direction(hb_buffer, HB_DIRECTION_LTR);
  // hb_buffer_set_script(hb_buffer, HB_SCRIPT_LATIN);
  // hb_buffer_set_language(hb_buffer, hb_language_from_string("en", -1));

  // hb_shape(hb_font, hb_buffer, nullptr, 0);
  // hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
  // hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);
  // unsigned int len = hb_buffer_get_length(hb_buffer);
  // for (unsigned int i = 0; i < len; i++) {
  //   fprintf(stderr, "cp: %d, msk: %d, cl: %d, xa: %d (%d) ya: %d xo: %d yo: %d\n",
  //           info[i].codepoint, info[i].mask, info[i].cluster,
  //           pos[i].x_advance,
  //           hb_font_get_glyph_h_advance(hb_font, info[i].codepoint),
  //           pos[i].y_advance,
  //           pos[i].x_offset, pos[i].y_offset);
  // }
}

}  // namespace {}

void PDFDoc::PlaceText(int pageno, SkPoint pagept, const std::string& ascii) {
  TestShape();
  ScopedPage page = pagecache_.OpenPage(pageno);
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
  std::string message = StrToUTF16LE(ascii);
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
  page.MarkDirty();
  // Handle Undo
  int index = FPDFPage_CountObjects(page.get()) - 1;
  undo_manager_.PushUndoOp(
      [this, pageno, index] () {
        DeleteObject(pageno, index);
      });
  SkRect dirty;
  if (FPDFPageObj_GetBounds(textobj, &dirty.fLeft, &dirty.fTop,
                            &dirty.fRight, &dirty.fBottom)) {
    render_cache.Invalidate(pageno, dirty);
    for (PDFDocEventHandler* handler : event_handlers_)
      handler->NeedsDisplayInRect(pageno, dirty);
  } else {
    fprintf(stderr, "Failed to get bounds for new text obj\n");
  }
}

void PDFDoc::UpdateText(int pageno, int index, const std::string& ascii,
                        const std::string& orig_value, bool undo) {
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return;
  }
  for (PDFDocEventHandler* handler : event_handlers_)
    handler->NeedsDisplayForObj(pageno, index);
  FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page.get(), index);
  if (!obj) {
    fprintf(stderr, "Page has no object at index %d\n", index);
    return;
  }
  std::string message = StrToUTF16LE(ascii.empty() ? " " : ascii);
  FPDF_WIDESTRING pdf_str = reinterpret_cast<FPDF_WIDESTRING>(message.c_str());
  if (!FPDFText_SetText(obj, pdf_str)) {
    fprintf(stderr, "failed to set text\n");
    return;
  }
  
  page.MarkDirty();
  render_cache.Invalidate(pageno, BoundingBoxForObj(pageno, index));
  for (PDFDocEventHandler* handler : event_handlers_)
    handler->NeedsDisplayForObj(pageno, index);
  if (undo) {
    undo_manager_.PushUndoOp(
        [this, pageno, index, ascii, orig_value] () {
          UpdateText(pageno, index, orig_value, ascii, true);
        });
  }
}

void PDFDoc::InsertFreehandDrawing(int pageno, const std::vector<SkPoint>& bezier) {
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return;
  }
  ScopedFPDFPageObject path(FPDFPageObj_CreateNewPath(bezier[0].x(),
                                                      bezier[0].y()));
  for (size_t i = 1; (i + 2) < bezier.size(); i += 3) {
    if (!FPDFPath_BezierTo(path.get(), bezier[i].x(), bezier[i].y(),
                           bezier[i + 1].x(), bezier[i + 1].y(),
                           bezier[i + 2].x(), bezier[i + 2].y())) {
      fprintf(stderr, "Failed to do BezierTo\n");
      return;
    }
  }
  if (!FPDFPath_SetStrokeColor(path.get(), 0, 0, 0, 250)) {
    fprintf(stderr, "FPDFPath_SetStrokeColor failed!\n");
    return;
  }
  if (!FPDFPath_SetStrokeWidth(path.get(), 1)) {
    fprintf(stderr, "FPDFPath_SetStrokeWidth failed!\n");
    return;
  }
  if (!FPDFPath_SetDrawMode(path.get(), 0, 1)) {
    fprintf(stderr, "FPDFPath_SetDrawMode failed!\n");
    return;
  }
  FPDF_PAGEOBJECT unowned_path = path.release();
  FPDFPage_InsertObject(page.get(), unowned_path);
  page.MarkDirty();
  // Handle Undo
  int index = FPDFPage_CountObjects(page.get()) - 1;
  undo_manager_.PushUndoOp(
      [this, pageno, index] () {
        DeleteObject(pageno, index);
      });
  SkRect dirty;
  if (FPDFPageObj_GetBounds(unowned_path, &dirty.fLeft, &dirty.fTop,
                            &dirty.fRight, &dirty.fBottom)) {
    render_cache.Invalidate(pageno, dirty);
    for (PDFDocEventHandler* handler : event_handlers_)
      handler->NeedsDisplayInRect(pageno, dirty);
  } else {
    fprintf(stderr, "Failed to get bounds for new path obj\n");
  }
}

void PDFDoc::MoveObjects(int pageno, const std::set<int>& objs,
                         float dx, float dy, bool do_move, bool do_undo) {
  if (do_move) {
    ScopedPage page = pagecache_.OpenPage(pageno);
    if (!page) {
      fprintf(stderr, "failed to load PDFPage\n");
      return;
    }
    for (int index : objs) {
      // Redraw the old location
      render_cache.Invalidate(pageno, BoundingBoxForObj(pageno, index));
      for (PDFDocEventHandler* handler : event_handlers_)
        handler->NeedsDisplayForObj(pageno, index);
      FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page.get(), index);
      if (!obj) {
        fprintf(stderr, "Page has no object at index %d\n", index);
        return;
      }
      FPDFPageObj_Transform(obj, 1, 0, 0, 1, dx, -dy);
    }
    page.MarkDirty();
    // Redraw new location, too
    for (int index : objs) {
      render_cache.Invalidate(pageno, BoundingBoxForObj(pageno, index));
      for (PDFDocEventHandler* handler : event_handlers_)
        handler->NeedsDisplayForObj(pageno, index);
    }
  }
  if (do_undo) {
    undo_manager_.PushUndoOp(
        [this, pageno, objs, dx, dy] () {
          MoveObjects(pageno, objs, -dx, -dy, true, true);
        });
  }
}

void PDFDoc::SetObjectBounds(int pageno, int objindex, SkRect bounds) {
  
}

void PDFDoc::DumpAPAtPagePt(int pageno, SkPoint pt) {
  ScopedPage page = pagecache_.OpenPage(pageno);
  if (!page) {
    fprintf(stderr, "failed to load PDFPage\n");
    return;
  }
  int cnt = FPDFPage_GetAnnotCount(page.get());
  fprintf(stderr, "There are %d annots to consider\n", cnt);
  for (int i = 0; i < cnt; i++) {
    //FPDF_ANNOT_FREETEXT
    FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page.get(), i);
    if (!annot)
      continue;
    FPDF_ANNOTATION_SUBTYPE type = FPDFAnnot_GetSubtype(annot);
    fprintf(stderr, "annot %d has type %d\n", i, type);
    unsigned long len =
        FPDFAnnot_GetAP(annot, FPDF_ANNOT_APPEARANCEMODE_NORMAL, nullptr, 0);
    if (len > 0) {
      unsigned char buf[len + 2];
      FPDFAnnot_GetAP(annot, FPDF_ANNOT_APPEARANCEMODE_NORMAL,
                      reinterpret_cast<FPDF_WCHAR *>(buf), len);
      buf[len] = buf[len + 1] = '\0';
      fprintf(stderr, "AP: %s\n", UTF16LEToStr(buf).c_str());
    }
    FPDFPage_CloseAnnot(annot);
  }
}

void PDFDoc::MovePages(int start, int end, int to) {
  if (to > start && to <= end)
    to = start;
  int ranges[4] = {start, end, 0, 0};
  pagecache_.RemoveAllObjects();
  FPDFPage_Move(doc_.get(), ranges, to);

  // Generate undo operation
  int len = end - start;
  if (to > start)
    to -= len;
  else
    start += len;
  render_cache.InvalidateAll();
  undo_manager_.PushUndoOp(
      [this, start, len, to] () {
        MovePages(to, to + len, start);
      });
}

void PDFDoc::MovePages(const std::vector<std::pair<int, int>>& from, int to) {
  // Split ranges into separate operations
  {
    // First, rewind 'to' to the start of a range if it falls in the middle
    // of one.
    auto it = std::lower_bound(from.begin(), from.end(), std::make_pair(0, to),
                               [] (const std::pair<int, int>& a,
                                   const std::pair<int, int>& b) -> bool {
                                 return a.second < b.second;
                               });
    if (it != from.end() && to > it->first) {
      to = it->first;
    }
  }
  ScopedUndoManagerGroup undo_group(&undo_manager_);
  // Next, start doing each operation
  int delta = 0;
  for (auto it = from.rbegin(); it != from.rend(); ++it) {
    if (to > it->first) {
      MovePages(it->first, it->second, to);
      continue;
    }
    if (to == (it->first + delta))
      continue;
    int len = it->second - it->first;
    MovePages(it->first + delta, it->second + delta, to);
    delta += len;
  }
}

void PDFDoc::DownloadDoc() const {
  // Remember objects in each page and save annotations
  std::vector<int> objects_per_page(Pages());
  for (size_t i = 0; i < objects_per_page.size(); i++) {
    ScopedPage page = pagecache_.OpenPage(i);
    if (!page) {
      fprintf(stderr, "failed to load PDFPage\n");
      return;
    }
    objects_per_page[i] = FPDFPage_CountObjects(page.get());
    for (PDFDocEventHandler* handler : event_handlers_) {
      if (handler->FlushAnnotations(doc_.get(), page.get(), i)) {
        page.MarkDirty();
      }
    }
  }
  pagecache_.GenerateContentStreams();
  FileSaver fs;
  if (!FPDF_SaveAsCopy(doc_.get(), &fs, FPDF_REMOVE_SECURITY)) {
    fprintf(stderr, "FPDF_SaveAsCopy failed\n");
    return;
  }
  bridge_downloadBytes(&fs.data_[0], fs.data_.size());

  // Clean up each page by deleting the new objects
  for (size_t i = 0; i < objects_per_page.size(); i++) {
    ScopedPage page = pagecache_.OpenPage(i);
    if (!page) {
      fprintf(stderr, "failed to load PDFPage\n");
      return;
    }
    while (true) {
      int obj_cnt = FPDFPage_CountObjects(page.get());
      if (obj_cnt <= objects_per_page[i])
        break;
      FPDF_PAGEOBJECT pageobj = page.GetObject(obj_cnt - 1);
      if (!pageobj) {
        fprintf(stderr, "Unable to get page obj\n");
        return;
      }
      if (!FPDFPage_RemoveObject(page.get(), pageobj)) {
        fprintf(stderr, "Unable to remove object\n");
        return;
      }
    }
  }
}

void PDFDoc::AppendPDF(const char* bytes, size_t length) {
  TestLoader loader({bytes, length});
  FPDF_FILEACCESS file_access = {length, TestLoader::GetBlock, &loader};
  ScopedFPDFDocument doc(FPDF_LoadCustomDocument(&file_access, nullptr));
  if (!doc) {
    PrintLastError();
    return;
  }
  if (!FPDF_DocumentHasValidCrossReferenceTable(doc.get()))
    fprintf(stderr, "Document has invalid cross reference table\n");
  (void)FPDF_GetDocPermissions(doc.get());

  if (!FPDF_ImportPages(doc_.get(), doc.get(), nullptr, Pages())) {
    fprintf(stderr, "FPDF_ImportPages failed!\n");
    return;
  }
  render_cache.InvalidateAll();
  for (PDFDocEventHandler* handler : event_handlers_)
    handler->PagesChanged();
}

}  // namespace formulate
