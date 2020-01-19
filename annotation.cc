// Copyright...

#include "annotation.h"

#include <codecvt>
#include <locale>
#include <string.h>

#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_edit.h"
#include "public/fpdf_ppo.h"
#include "SkPDFDocument.h"

namespace formulate {

namespace {
const double kMaxUnboundedTextWidth = 72.0 * 8.5;
} // namespace {}

Annotation::~Annotation() {
  if (dirty_)
    fprintf(stderr, "ERR: Deleting dirty annotation\n");
}

Annotation* Annotation::Create(AnnotationDelegate* delegate,
                               Toolbox::Tool type) {
  switch (type) {
    case Toolbox::kText_Tool:
      return new TextAnnotation(delegate);
    case Toolbox::kFreehand_Tool:
      return new PathAnnotation(delegate);
    default:
      fprintf(stderr, "can't create annotation for type %d\n", type);
      return nullptr;
  }
}

void Annotation::CreateMouseDown(SkPoint pt) {
  down_pt_ = pt;
  bounds_ = SkRect::MakeLTRB(pt.x(), pt.y(), pt.x(), pt.y());
}

void Annotation::CreateMouseDrag(SkPoint pt) {
  bounds_.set(pt, down_pt_);
}

bool Annotation::CreateMouseUp(SkPoint pt) {
  bounds_.set(pt, down_pt_);
  // Only keep Annotations that are big enough:
  return bounds_.width() > 2.0 && bounds_.height() > 2.0;
}

void Annotation::SetHoverPoint(SkPoint pt) {
  // Default: center annotation at point
  float width = bounds_.width();
  float height = bounds_.height();
  bounds_.fLeft = pt.x() - width / 2;
  bounds_.fTop = pt.y() - height / 2;
  SetWidth(width);
  SetHeight(height);
}

void Annotation::Move(float dx, float dy) {
  bounds_.offset(dx, dy);
}

void Annotation::MoveKnob(Knobmask knob, float dx, float dy) {
  switch (knob) {
    default:
      fprintf(stderr, "Illegal knob passed 0x%08x\n", knob);
      return;
    case kTopLeftKnob:
    case kMiddleLeftKnob:
    case kBottomLeftKnob:
      bounds_.fLeft += dx;
      break;
    case kTopCenterKnob:
    case kBottomCenterKnob:
      break;
    case kTopRightKnob:
    case kMiddleRightKnob:
    case kBottomRightKnob:
      bounds_.fRight += dx;
      break;
  }

  switch (knob) {
    default:
      fprintf(stderr, "Illegal knob passed 0x%08x\n", knob);
      return;
    case kTopLeftKnob:
    case kTopCenterKnob:
    case kTopRightKnob:
      bounds_.fTop += dy;
      break;
    case kMiddleLeftKnob:
    case kMiddleRightKnob:
      break;
    case kBottomLeftKnob:
    case kBottomCenterKnob:
    case kBottomRightKnob:
      bounds_.fBottom += dy;
      break;
  }
}

void Annotation::DrawKnobs(SkCanvas* canvas, SkRect rect) {
  // TODO(adlr): draw the knobs
}

SkRect Annotation::BoundsWithKnobs() const {
  SkRect ret = Bounds();
  // TODO(adlr): add bounds for knobs
  return ret;
}

SkRect Annotation::KnobBounds(char knob) {
  // For now, a dummy value:
  SkPoint quad[4];
  bounds_.toQuad(quad);
  return SkRect::MakeLTRB(quad[0].x() - 2,
                          quad[0].y() - 2,
                          quad[2].x() + 2,
                          quad[2].y() + 2);
}

TextAnnotation::TextAnnotation(AnnotationDelegate* delegate)
    : Annotation(delegate) {
  paragraph_ = delegate_->ParseText("Text");
  LayoutText();  // computes height and saves it into bounds_
}

std::u16string UTF8To16(const std::string& str) {
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
  return utf16conv.from_bytes(str);
}

std::string UTF16LETo8(const char* bytes, unsigned long numbytes) {
  // Note: assumes machine is little endian
  return std::wstring_convert<
    std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(
        reinterpret_cast<const char16_t*>(bytes),
        reinterpret_cast<const char16_t*>(bytes + numbytes));
}

Annotation* AnnotationFromPageObject(AnnotationDelegate* delegate,
                                     FPDF_PAGE page,
                                     FPDF_PAGEOBJECT obj) {
  const int markcnt = FPDFPageObj_CountMarks(obj);
  if (markcnt < 0) {
    fprintf(stderr, "Failed to get mark count from page object\n");
  }
  if (markcnt == 0)
    return nullptr;
  for (int i = 0; i < markcnt; i++) {
    FPDF_PAGEOBJECTMARK mark = FPDFPageObj_GetMark(obj, i);
    if (!mark) {
      fprintf(stderr, "Failed to get mark from page obj\n");
      break;
    }
    char name_bytes_16le[100] = {0};  // should be enough for any name, right?
    unsigned long out_buflen = 0;
    if (!FPDFPageObjMark_GetName(mark, name_bytes_16le, sizeof(name_bytes_16le),
                                 &out_buflen) || out_buflen == 0) {
      fprintf(stderr, "Failed to get mark name length\n");
      break;
    }
    if (sizeof(TextAnnotation::kSaveKeyUTF16LE) == out_buflen &&
        !memcmp(TextAnnotation::kSaveKeyUTF16LE, name_bytes_16le, out_buflen)) {
      return new TextAnnotation(delegate, page, obj, mark);
    }
  }
  return nullptr;
}

std::vector<std::unique_ptr<Annotation>> AnnotationsFromPage(
    AnnotationDelegate* delegate, FPDF_PAGE page) {
  std::vector<std::unique_ptr<Annotation>> ret;
  const int objcnt = FPDFPage_CountObjects(page);
  for (int i = objcnt - 1; i > 0; i--) {
    FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, i);
    if (!obj) {
      fprintf(stderr, "Got null object!\n");
      break;
    }
    Annotation* annotation = AnnotationFromPageObject(delegate, page, obj);
    if (!annotation)
      break;
    ret.emplace_back(annotation);
    if (!FPDFPage_RemoveObject(page, obj)) {
      fprintf(stderr, "Unable to remove object during load\n");
      break;
    }
  }
  std::reverse(ret.begin(), ret.end());
  return ret;
}

TextAnnotation::TextAnnotation(AnnotationDelegate* delegate,
                               FPDF_PAGE page,
                               FPDF_PAGEOBJECT obj,
                               FPDF_PAGEOBJECTMARK mark)
    : Annotation(delegate) {
  // Get string value
  unsigned long out_buflen = 0;
  if (!FPDFPageObjMark_GetParamStringValue(
          mark, "V", nullptr, 0, &out_buflen)) {
    fprintf(stderr, "Can't get value len from marked content\n");
    return;
  }
  std::vector<char> buf(out_buflen);
  if (!FPDFPageObjMark_GetParamStringValue(
          mark, "V", &buf[0], buf.size(), &out_buflen)) {
    fprintf(stderr, "Can't get value from marked content\n");
    return;
  }
  editing_value_ = UTF16LETo8(&buf[0], buf.size());

  int width = 0;
  if (FPDFPageObjMark_GetParamIntValue(mark, "W", &width) && width >= 0) {
    fixed_width_ = true;
  }
  // Adjust bounds based on how big the text is.
  double a, b, c, d, e, f;
  if (!FPDFFormObj_GetMatrix(obj, &a, &b, &c, &d, &e, &f)) {
    fprintf(stderr, "unable to get form obj matrix\n");
    return;
  }
  bounds_.fLeft = e;
  bounds_.fTop = 0;
  bounds_.fRight = e + width + 0.1;
  bounds_.fBottom = 0;
  paragraph_ = delegate_->ParseText(editing_value_.c_str());
  LayoutText();  // computes height and saves it into bounds_

  double page_height = FPDF_GetPageHeight(page);
  double height = bounds_.height();
  bounds_.fTop = page_height - height - f;
  bounds_.fBottom = bounds_.fTop + height;
}

TextAnnotation::~TextAnnotation() {
}

bool TextAnnotation::CreateMouseUp(SkPoint pt) {
  bool keep = Annotation::CreateMouseUp(pt);
  // Always keep TextAnnotations, but for small ones, just make the bounds
  // into a point
  if (!keep) {
    bounds_.fRight = bounds_.fLeft;
    bounds_.fBottom = bounds_.fTop;
  }
  return true;
}

void TextAnnotation::SetHoverPoint(SkPoint pt) {
  float width = bounds_.width();
  float height = bounds_.height();
  bounds_.fLeft = pt.x();
  bounds_.fTop = pt.y();
  SetWidth(width);
  SetHeight(height);
}

void TextAnnotation::Draw(SkCanvas* canvas, SkRect rect) {
  if (editing_)
    return;
  if (!paragraph_) {
    fprintf(stderr, "No paragraph to draw\n");
    return;
  }
  // paragraph_->Layout(Bounds().width());
  canvas->save();
  canvas->translate(Bounds().left(), Bounds().top());
  paragraph_->Paint(canvas, 0, 0);
  canvas->restore();
}

void TextAnnotation::Flush(FPDF_DOCUMENT doc, FPDF_PAGE page) {
  SkDynamicMemoryWStream mem_stream;
  {
    // Create temp PDF, write to mem_stream, then destroy it
    SkPDF::Metadata metadata;
    auto pdf = SkPDF::MakeDocument(&mem_stream, metadata);
    if (!pdf.get()) {
      fprintf(stderr, "couldn't create PDF document\n");
      return;
    }
    SkCanvas* page_canvas = pdf->beginPage(bounds_.width(), bounds_.height());
    if (!page_canvas) {
      fprintf(stderr, "couldn't create page canvas\n");
      return;
    }
    page_canvas->save();
    page_canvas->translate(-Bounds().left(), -Bounds().top());
    Draw(page_canvas, Bounds());
    page_canvas->restore();
    pdf->endPage();
    pdf->close();
  }
  // Open the new PDF in pdfium and convert to Form XObject
  sk_sp<SkData> src_bytes = mem_stream.detachAsData();
  fprintf(stderr, "temp PDF is %zu bytes in size\n", src_bytes->size());
  ScopedFPDFDocument src(FPDF_LoadMemDocument(
      src_bytes->data(),
      static_cast<int>(src_bytes->size()),
      nullptr));
  if (!src) {
    fprintf(stderr, "Can't open temp PDF in pdfium\n");
    return;
  }
  ScopedFPDFPageObject form_object(
      FPDF_ImportPageToXObject(doc, src.get(), 1));
  if (!form_object) {
    fprintf(stderr, "failed to import pdf to form xobject\n");
    return;
  }

  // Set marked content value on the object
  FPDF_PAGEOBJECTMARK mark = FPDFPageObj_AddMark(form_object.get(),
                                                 "FN:RichText");
  if (!mark) {
    fprintf(stderr, "failed to create mark\n");
    return;
  }
  if (!FPDFPageObjMark_SetStringParam(
          doc, form_object.get(), mark, "V", editing_value_.c_str())) {
    fprintf(stderr, "failed to set mark str value\n");
    return;
  }
  if (fixed_width_) {
    if (!FPDFPageObjMark_SetIntParam(
            doc, form_object.get(), mark, "W", bounds_.width())) {
      fprintf(stderr, "failed to set mark int value\n");
      return;
    }
  }

  // Position the object on the page and add to the page
  double page_height = FPDF_GetPageHeight(page);
  double y_bottom = page_height - Bounds().height() - Bounds().top();
  FPDFPageObj_Transform(form_object.get(),
                        1, 0, 0, 1, Bounds().left(), y_bottom);
  FPDFPage_InsertObject(page, form_object.release());
}

void TextAnnotation::StartEditing(SkPoint pt) {
  editing_ = true;
}

void TextAnnotation::StopEditing() {
  editing_ = false;
  paragraph_ = delegate_->ParseText(editing_value_.c_str());
  LayoutText();
  // set needs display
}

void TextAnnotation::LayoutText() {
  // Re-layout text
  paragraph_->Layout(fixed_width_ ? bounds_.width() : kMaxUnboundedTextWidth);
  if (!fixed_width_) {
    double longest_line = paragraph_->GetLongestLine();
    SetWidth(ceilf(longest_line) + 0.1);
  }
  SetHeight(paragraph_->GetHeight());
}

void TextAnnotation::MoveKnob(Knobmask knob, float dx, float dy) {
  Annotation::MoveKnob(knob, dx, dy);
  if (knob & (kTopLeftKnob |
              kTopRightKnob |
              kMiddleLeftKnob |
              kMiddleRightKnob |
              kBottomLeftKnob |
              kBottomRightKnob)) {
    fixed_width_ = true;
  }
  LayoutText();
}

PathAnnotation::PathAnnotation(AnnotationDelegate* delegate)
    : Annotation(delegate) {
  paint_.setAntiAlias(true);
  paint_.setStyle(SkPaint::kStroke_Style);
  paint_.setColor(0xff000000);  // opaque black
  paint_.setStrokeWidth(1);
}

// Load a PathAnnotation from saved PDF:
PathAnnotation::PathAnnotation(AnnotationDelegate* delegate,
                               FPDF_PAGE page,
                               FPDF_PAGEOBJECT obj,
                               FPDF_PAGEOBJECTMARK mark)
    : Annotation(delegate) {
}

PathAnnotation::~PathAnnotation() {}

void PathAnnotation::CreateMouseDown(SkPoint pt) {
  fprintf(stderr, "called PathAnnotation::CreateMouseDown\n");
  fresh_ = true;
  path_.moveTo(pt);
  prev_pt_.fX = prev_pt_.fY = -1;  // indicates unset
  bounds_.fLeft = bounds_.fRight = pt.x();
  bounds_.fTop = bounds_.fBottom = pt.y();
}

namespace {
// From https://gist.github.com/njvack/6925609
// |pts| point to an array of 4 points. Control points for the middle
// segment are returned.
// For the first segment, pass the first point twice. For the last segment,
// pass the last point twice.
std::pair<SkPoint, SkPoint> ControlPoints(const SkPoint* pts) {
  std::pair<SkPoint, SkPoint> ret;
  ret.first.fX = (-pts[0].x() + 6*pts[1].x() + pts[2].x()) / 6;
  ret.first.fY = (-pts[0].y() + 6*pts[1].y() + pts[2].y()) / 6;
  ret.second.fX = (pts[1].x() + 6*pts[2].x() - pts[3].x()) / 6;
  ret.second.fY = (pts[1].y() + 6*pts[2].y() - pts[3].y()) / 6;
  return ret;
}
}  // namespace {}

void PathAnnotation::CreateMouseDrag(SkPoint pt) {
  AppendPointInCreate(pt, false);
}

void PathAnnotation::AppendPointInCreate(SkPoint pt, bool force) {
  // First, find out how many points we have so far. We know we must
  // have at least one from the CreateMouseDown event. We also don't
  // need more than 3 existing ones, so don't look beyond that.
  SkPoint points[4] = {
    SkPoint::Make(0, 0),
    SkPoint::Make(0, 0),
    prev_pt_,
    pt
  };
  path_.getLastPt(&points[1]);
  if (path_.countPoints() > 1) {
    if (path_.countPoints() <= 3)
      return;
    points[0] = path_.getPoint(path_.countPoints() - 4);
  } else {
    points[0] = points[1];
  }
  fprintf(stderr, "4 pts: %f %f %f %f\n",
          points[0].x(),
          points[1].x(),
          points[2].x(),
          points[3].x());

  if (prev_pt_.fX < 0) {
    // TODO(adlr): check for repeated point
    prev_pt_ = pt;
    return;
  }

  if (prev_pt_ == pt)
    return;

  // We have a least 3 points now, so add a bezier path segment
  std::pair<SkPoint, SkPoint> ctrl_pts = std::make_pair(points[1], points[2]);
            //ControlPoints(points);
  fprintf(stderr, "about to maybe malloc\n");
  path_.cubicTo(ctrl_pts.first, ctrl_pts.second, prev_pt_);
  prev_pt_ = pt;
  fprintf(stderr, "did maybe malloc\n");
  bounds_ = path_.getBounds();  // slightly incorrect but fast
}

bool PathAnnotation::CreateMouseUp(SkPoint pt) {
  AppendPointInCreate(pt, false);
  AppendPointInCreate(pt, true);
  bounds_ = path_.computeTightBounds();
  return path_.countPoints() > 1;
}

namespace {
class BezierIterator {
 public:
  BezierIterator(SkPoint* points, size_t size)
      : points_(points), cnt_(size), i_(1) {}
  bool HasNext() { return (i_ + 1) < cnt_; }
  void Get(SkPoint* out) {
    std::pair<SkPoint, SkPoint> cpx;
    if (i_ == 1 || i_ + 1 == cnt_) {
      SkPoint pts[4] = {
        i_ == 1 ? points_[0] : points_[i_ - 2],
        i_ == 1 ? points_[0] : points_[i_ - 1],
        points_[i_],
        i_ + 1 == cnt_ ? points_[i_] : points_[i_ + 1]
      };
      cpx = ControlPoints(pts);
    } else {
      cpx = ControlPoints(&points_[i_ - 2]);
    }
    out[0] = cpx.first;
    out[1] = cpx.second;
    out[2] = points_[i_];
  }
  void Inc() { i_++; }

 private:
  SkPoint* points_;
  size_t cnt_;
  size_t i_;
};

}  // namespace {}

void PathAnnotation::Draw(SkCanvas* canvas, SkRect rect) {
  canvas->drawPath(path_, paint_);

  // if (points_.size() < 2)
  //   return;

  // SkPath path;
  // float sx = 1.0, sy = 1.0;
  // float tx = 0.0, ty = 0.0;

  // if (!placing_) {
  //   sx = bounds_.width();
  //   sy = bounds_.height();
  //   tx = bounds_.left();
  //   ty = bounds_.top();
  // }


  // path.moveTo(points_[0].x() * sx + tx, points_[0].y() * sy + ty);

  // for (BezierIterator it(&points_[0], points_.size());
  //      it.HasNext(); it.Inc()) {
  //   SkPoint pts[3];
  //   it.Get(pts);
  //   path.cubicTo(pts[0].x() * sx + tx,
  //                pts[0].y() * sy + ty,
  //                pts[1].x() * sx + tx,
  //                pts[1].y() * sy + ty,
  //                pts[2].x() * sx + tx,
  //                pts[2].y() * sy + ty);
  // }

  // SkPaint paint;
  // paint.setAntiAlias(true);
  // paint.setStyle(SkPaint::kStroke_Style);
  // paint.setColor(0xff000000);  // opaque black
  // paint.setStrokeWidth(1);

  // canvas->drawPath(path, paint);
}

void PathAnnotation::Flush(FPDF_DOCUMENT doc, FPDF_PAGE page) {
  // if (points_.size() <= 1 || placing_)
  //   return;

  // float sx = bounds_.width();
  // float sy = bounds_.height();
  // float tx = bounds_.left();
  // float ty = bounds_.top();
  // ScopedFPDFPageObject path(
  //     FPDFPageObj_CreateNewPath(points_[0].x() * sx + tx,
  //                               points_[0].y() * sy + ty));
  
  // if (!FPDFPath_SetStrokeColor(path.get(), 0, 0, 0, 0.5)) {
  //   fprintf(stderr, "FPDFPath_SetStrokeColor failed!\n");
  //   return;
  // }
  // if (!FPDFPath_SetDrawMode(path.get(), FPDF_FILLMODE_NONE, true)) {
  //   fprintf(stderr, "FPDFPath_SetDrawMode failed\n");
  //   return;
  // }

  // if (!FPDFPath_SetStrokeWidth(path.get(), 1.0)) {
  //   fprintf(stderr, "FPDFPath_SetStrokeWidth failed\n");
  //   return;
  // }

  // for (BezierIterator it(&points_[0], points_.size());
  //      it.HasNext(); it.Inc()) {
  //   SkPoint pts[3];
  //   it.Get(pts);
  //   if (!FPDFPath_BezierTo(path.get(), pts[0].x() * sx + tx,
  //                          pts[0].y() * sy + ty,
  //                          pts[1].x() * sx + tx,
  //                          pts[1].y() * sy + ty,
  //                          pts[2].x() * sx + tx,
  //                          pts[2].y() * sy + ty)) {
  //     fprintf(stderr, "FPDFPath_BezierTo failed\n");
  //     return;
  //   }
  // }
  // FPDFPage_InsertObject(page, path.release());
}

}  // namespace formulate
