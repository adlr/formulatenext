// Copyright...

#include "annotation.h"

namespace formulate {

namespace {
const double kMaxUnboundedTextWidth = 72.0 * 8.5;
} // namespace {}

Annotation::~Annotation() {
  fprintf(stderr, "deleted annotation\n");
  if (dirty_)
    fprintf(stderr, "ERR: Deleting dirty annotation\n");
}

Annotation* Annotation::Create(AnnotationDelegate* delegate,
                               Toolbox::Tool type) {
  switch (type) {
    case Toolbox::kText_Tool:
      fprintf(stderr, "creating text annotation\n");
      return new TextAnnotation(delegate);
    default:
      fprintf(stderr, "can't create annotation for type %d\n", type);
      return nullptr;
  }
}

void Annotation::CreateMouseDown(int page, SkPoint pt) {
  page_ = page;
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

TextAnnotation::~TextAnnotation() {
  fprintf(stderr, "deleted text annotation\n");
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

void TextAnnotation::Draw(SkCanvas* canvas, SkRect rect) {
  if (editing_)
    return;
  if (!paragraph_) {
    fprintf(stderr, "No paragraph to draw\n");
    return;
  }
  paragraph_->Layout(Bounds().width());
  canvas->save();
  canvas->translate(Bounds().left(), Bounds().top());
  paragraph_->Paint(canvas, 0, 0);
  canvas->restore();
}

void TextAnnotation::Flush() {
  fprintf(stderr, "TODO: save text annotation to PDF\n");
}

void TextAnnotation::StartEditing(SkPoint pt) {
  fprintf(stderr, "called TA::StartEditing\n");
  if (!delegate_) {
    fprintf(stderr, "No delegate. Can't start editing.\n");
    return;
  }
  editing_ = true;
  // TODO(adlr): incorporate pt to cursor position
  delegate_->StartComposingText(
      page(),
      SkPoint::Make(bounds_.left(),
                    bounds_.top()),
      fixed_width_ ? bounds_.width() : 0,
      editing_value_.c_str(),
      0,
      [this] (const char* new_text) {
        if (!editing_) {
          fprintf(stderr, "not editing text!\n");
          return;
        }
        editing_value_ = new_text;
      });
}

void TextAnnotation::StopEditing() {
  if (!delegate_) {
    fprintf(stderr, "No delegate. Can't stop editing.\n");
    return;
  }
  delegate_->StopEditingText();
  editing_ = false;

  paragraph_ = delegate_->ParseText(editing_value_.c_str());
  // Re-layout text
  fprintf(stderr, "using width: %f\n",
          fixed_width_ ? bounds_.width() : kMaxUnboundedTextWidth);
  paragraph_->Layout(fixed_width_ ? bounds_.width() : kMaxUnboundedTextWidth);
  if (!fixed_width_)
    SetWidth(ceilf(paragraph_->GetLongestLine()));
  SetHeight(paragraph_->GetHeight());
  fprintf(stderr, "size: %f %f\n", bounds_.width(), bounds_.height());

  // set needs display
}

}  // namespace formulate
