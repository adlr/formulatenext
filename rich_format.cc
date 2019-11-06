// Copyright...

#include "rich_format.h"

#include <emscripten.h>

#include <hb.h>
#include <hb-ft.h>

extern "C" {

using formulate::HTMLNodeWalkerInterface;

EMSCRIPTEN_KEEPALIVE
void StaticHTMLText(HTMLNodeWalkerInterface* self, const char* text) {
  fprintf(stderr, "StaticHTMLText\n");
  self->HTMLText(text);
}

EMSCRIPTEN_KEEPALIVE
void StaticHTMLNodeStarted(HTMLNodeWalkerInterface* self,
                           const char* tag_name) {
  fprintf(stderr, "StaticHTMLNodeStarted\n");
  self->HTMLNodeStarted(tag_name);
}

EMSCRIPTEN_KEEPALIVE
void StaticHTMLNodeAttribute(HTMLNodeWalkerInterface* self,
                        const char* key, const char* value) {
  fprintf(stderr, "StaticHTMLNodeAttribute\n");
  self->HTMLNodeAttribute(key, value);
}

EMSCRIPTEN_KEEPALIVE
void StaticHTMLNodeEnded(HTMLNodeWalkerInterface* self) {
  fprintf(stderr, "StaticHTMLNodeEnded\n");
  self->HTMLNodeEnded();
}

}  // extern "C"

namespace formulate {

void HTMLWalk(const char* str, HTMLNodeWalkerInterface* callbacks) {
  // HTML parsing is a lot easier in the JS side
  EM_ASM_({
      HTMLWalk(UTF8ToString($0), $1);
    }, str, callbacks);
}

struct SkEmbeddedResource {const uint8_t* data; const size_t size;};
struct SkEmbeddedHeader {const SkEmbeddedResource* entries; const int count;};
extern "C" SkEmbeddedHeader const ARIMO_FONT;

namespace {
const int kFontExtraScale = 64;
}  // namespace {}

RichFormat::RichFormat() {
  style_.push_back(Style());

  FT_Error err = FT_Init_FreeType(&ftlib_);
  if (err) {
    fprintf(stderr, "FT_Init_FreeType failed\n");
    return;
  }
  err = FT_New_Memory_Face(ftlib_, ARIMO_FONT.entries[0].data,
                           ARIMO_FONT.entries[0].size, 0, &arimo_);
  if (err) {
    fprintf(stderr, "FT_New_Memory_Face failed\n");
    return;
  }
}

LayoutRow::CharIterator LayoutRow::CharBegin() {
  CharIterator ret(this);
  ret.str_it_ = elements_.begin();
  while (ret.str_it_ != elements_.end()) {
    if ((*ret.str_it_)->IsLayoutString() &&
        !(*ret.str_it_)->AsLayoutString()->chars_.empty()) {
      ret.char_it_ = (*ret.str_it_)->AsLayoutString()->chars_.begin();
      return ret;
    }
    ++ret.str_it_;
  }
  return ret;
}

LayoutRow::CharIterator LayoutRow::CharEnd() {
  CharIterator ret(this);
  ret.str_it_ = elements_.end();
  return ret;
}

void LayoutRow::CharIterator::Inc() {
  if (str_it_ == row_->elements_.end()) {
    fprintf(stderr, "Can't increment CharIterator. Already at end\n");
    return;
  }
  ++char_it_;
  if (char_it_ == (*str_it_)->AsLayoutString()->chars_.end()) {
    // move to next string
    int limit = 15;
    while (str_it_ != row_->elements_.end()) {
      if (--limit == 0) {
        fprintf(stderr, "break out of Inc loop\n");
        return;
      }
      ++str_it_;
      if (str_it_ == row_->elements_.end())
        return;
      if ((*str_it_)->IsLayoutString()) {
        char_it_ = (*str_it_)->AsLayoutString()->chars_.begin();
        if (char_it_ != (*str_it_)->AsLayoutString()->chars_.end()) {
          // Done!
          return;
        }
        // Got an empty string apparently, so keep looping.
      }
    }
  }
}

const Style& LayoutRow::CharIterator::Style() const {
  for (std::vector<std::unique_ptr<LayoutElement>>::iterator it = str_it_;
       ; --it) {
    if ((*it)->IsLayoutStyle())
      return (*it)->AsLayoutStyle()->style_;
    if (it == row_->elements_.begin())
      break;
  }
  fprintf(stderr, "Unable to find style for CharIterator\n");
  return Style();
}

const std::vector<LayoutRow>& RichFormat::Format(const char* html, float width) {
  HTMLWalk(html, this);
  return rows_;
}

FT_Face RichFormat::FT_FaceForStyle(const Style& style) {
  FT_Set_Char_Size(arimo_, 0, style.size_ * kFontExtraScale, 0, 0);
  return arimo_;
}

const char* RichFormat::PDFFontForStyle(const Style& style) {
  return "Helvetica";
}

std::pair<float, float> RichFormat::GetFullAscenderAndDescender(
    const FT_Face face) {
  float extra_leading = face->height - (face->ascender - face->descender);
  return std::make_pair(
      (face->ascender + extra_leading / 2) / kFontExtraScale,
      (face->descender - extra_leading / 2) / kFontExtraScale);
}

void RichFormat::HTMLText(const char* text) {
  std::string str;
  while (text[0] != '\0') {
    if (text[0] == '\n') {
      if (!str.empty()) {
        HandleStringWithoutNewline(str.c_str());
        str.clear();
      }
      HandleNewline();
    } else {
      str += text[0];
    }
    text++;
  }
  if (!str.empty())
    HandleStringWithoutNewline(str.c_str());
}

void RichFormat::HandleNewline() {
  rows_.resize(rows_.size() + 1);
  rows_.back().elements_.emplace_back(new LayoutStyle(style_.back()));
}

void RichFormat::HandleStringWithoutNewline(const char* text) {
  fprintf(stderr, "HandleStringWithoutNewline called: %s\n", text);
  if (rows_.empty())
    rows_.resize(1);

  const Style& style = style_.back();
  rows_.back().elements_.emplace_back(new LayoutStyle(style));

  FT_Face face = FT_FaceForStyle(style);

  hb_font_t* hb_font = hb_ft_font_create(face, nullptr);

  hb_font_set_scale(hb_font, style.size_ * kFontExtraScale,
                    style.size_ * kFontExtraScale);
  hb_buffer_t *hb_buffer = hb_buffer_create();
  hb_buffer_add_utf8(hb_buffer, text, -1, 0, -1);
  hb_buffer_guess_segment_properties(hb_buffer);

  hb_buffer_set_direction(hb_buffer, HB_DIRECTION_LTR);
  hb_buffer_set_script(hb_buffer, HB_SCRIPT_LATIN);
  hb_buffer_set_language(hb_buffer, hb_language_from_string("en", -1));

  hb_shape(hb_font, hb_buffer, nullptr, 0);
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);
  unsigned int len = hb_buffer_get_length(hb_buffer);
  for (unsigned int i = 0; i < len; i++) {
    fprintf(stderr, "cp: %d, msk: %d, cl: %d, xa: %d (%d) ya: %d xo: %d yo: %d\n",
            info[i].codepoint, info[i].mask, info[i].cluster,
            pos[i].x_advance,
            hb_font_get_glyph_h_advance(hb_font, info[i].codepoint),
            pos[i].y_advance,
            pos[i].x_offset, pos[i].y_offset);
    struct LayoutString::LayoutChar layout_char({
        static_cast<float>(pos[i].x_advance) / kFontExtraScale,
            static_cast<float>(hb_font_get_glyph_h_advance(hb_font,
                                                           info[i].codepoint)) /
            kFontExtraScale,
            CharcodeFromGlyph(face, info[i].codepoint)});
    InsertCharOntoLastRow(layout_char);
  }
  bool need_break = true;
  int limit = 80;
  while (need_break) {
    fprintf(stderr, "about to line break\n");
    need_break = LineBreakLastRow();
    fprintf(stderr, "did line break\n");
    --limit;
    if (limit == 0) {
      fprintf(stderr, "hit limit outer\n");
      break;
    }
  }
}

bool RichFormat::LineBreakLastRow() {
  LayoutRow& last_row = rows_.back();
  float width = 0.0;
  Style last_style;
  // Last space before it went over the width threshold
  LayoutRow::CharIterator last_space = last_row.CharEnd();
  // The char that crossed over the width threshold
  LayoutRow::CharIterator overflow_char = last_row.CharEnd();
  bool need_break = false;

  // First, find the character that crosses over the width boundary
  int limit = 30;
  for (LayoutRow::CharIterator it = last_row.CharBegin();
       it != last_row.CharEnd(); it.Inc()) {
    if (--limit == 0) {
      fprintf(stderr, "break in first width check\n");
      return false;
    }
    if (it.char_it_->char_code_ == ' ')
      last_space = it;
    width += it.char_it_->advance_;
    if (width > width_ && it != last_row.CharBegin()) {
      need_break = true;
      overflow_char = it;
      break;
    }
  }

  if (!need_break)
    return false;

  limit = 80;

  // move last_space past any whitespace
  while (last_space != last_row.CharEnd() &&
         last_space.char_it_->char_code_ == ' ') {
    last_space.Inc();
    --limit;
    if (limit == 0) {
      fprintf(stderr, "hit limit 1\n");
      break;
    }
  }

  limit = 80;

  // move overflow_char past any whitespace
  while (overflow_char != last_row.CharEnd() &&
         overflow_char.char_it_->char_code_ == ' ') {
    overflow_char.Inc();
    --limit;
    if (limit == 0) {
      fprintf(stderr, "hit limit 2\n");
      break;
    }
  }
  
  if (last_space != last_row.CharEnd())
    overflow_char = last_space;

  if (overflow_char == last_row.CharEnd()) {
    fprintf(stderr, "no place to break line\n");
    return false;
  }
  
  fprintf(stderr, "before break:\n");
  last_row.Dump();

  // Do the break
  LayoutRow new_row;
  new_row.elements_.emplace_back(new LayoutStyle(overflow_char.Style()));
  std::unique_ptr<LayoutString> back_half(new LayoutString());
  back_half->chars_.insert(
      back_half->chars_.end(),
      overflow_char.char_it_,
      (*overflow_char.str_it_)->AsLayoutString()->chars_.end());
  fprintf(stderr, "back half: ");
  back_half->Dump();
  fprintf(stderr, "\n");
  (*overflow_char.str_it_)->AsLayoutString()->chars_.erase(
      overflow_char.char_it_,
      (*overflow_char.str_it_)->AsLayoutString()->chars_.end());
  if (!back_half->chars_.empty()) {
    new_row.elements_.emplace_back(std::move(back_half));
  }
  std::vector<std::unique_ptr<LayoutElement>>::iterator keep =
      overflow_char.str_it_;
  ++keep;
  fprintf(stderr, "doing insert %zu\n", last_row.elements_.size());
  new_row.elements_.insert(
      new_row.elements_.end(),
      std::make_move_iterator(keep),
      std::make_move_iterator(last_row.elements_.end()));
  fprintf(stderr, "doing erase %zu\n", last_row.elements_.size());
  last_row.elements_.erase(keep, last_row.elements_.end());
  fprintf(stderr, "doing push back %zu\n", last_row.elements_.size());
  rows_.push_back(std::move(new_row));  // invalidates last_row

  fprintf(stderr, "new last row:\n");
  rows_.back().Dump();

  return true;
}

void RichFormat::InsertCharOntoLastRow(
    struct LayoutString::LayoutChar layout_char) {
  LayoutRow& last_row = rows_.back();
  if (!last_row.elements_.back()->IsLayoutString()) {
    last_row.elements_.emplace_back(new LayoutString);
  }
  if (!last_row.elements_.back()->IsLayoutString()) {
    fprintf(stderr, "Can't find LayoutString!\n");
    return;
  }
  LayoutString* str = last_row.elements_.back()->AsLayoutString();
  str->PushChar(layout_char);
}

uint16_t RichFormat::CharcodeFromGlyph(FT_Face face, uint16_t glyph_id) {
  // TODO(adlr): be more efficient. There is certainly a way to avoid
  // this long loop for every char.
  FT_UInt iter_glyph_id = 0;
  FT_ULong iter_char = FT_Get_First_Char(face, &iter_glyph_id);
  while (iter_glyph_id != 0) {
    if (glyph_id == iter_glyph_id)
      return iter_char;
    iter_char = FT_Get_Next_Char(face, iter_char, &iter_glyph_id);
  }
  return 0;
}

void RichFormat::HTMLNodeStarted(const char* tag_name) {
  style_.push_back(*style_.rbegin());
  if (!strcmp(tag_name, "B"))
    style_.back().bold_ = true;
  if (!strcmp(tag_name, "I"))
    style_.back().italic_ = true;
  if (!strcmp(tag_name, "BR"))
    HTMLText("\n");
}

void RichFormat::HTMLNodeEnded() {
  if (style_.size() <= 1) {
    fprintf(stderr, "style_ would be empty!\n");
    return;
  }
  style_.pop_back();
}

}  // namespace formulate
