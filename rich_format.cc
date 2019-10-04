// Copyright...

#include "rich_format.h"

#include <emscripten.h>

#include <hb.h>
#include <hb-ft.h>

extern "C" {

using formulate::HTMLNodeWalkerInterface;

EMSCRIPTEN_KEEPALIVE
void StaticHTMLText(HTMLNodeWalkerInterface* self, const char* text) {
  self->HTMLText(text);
}

EMSCRIPTEN_KEEPALIVE
void StaticHTMLNodeStarted(HTMLNodeWalkerInterface* self,
                           const char* tag_name) {
  self->HTMLNodeStarted(tag_name);
}

EMSCRIPTEN_KEEPALIVE
void StaticHTMLNodeAttribute(HTMLNodeWalkerInterface* self,
                        const char* key, const char* value) {
  self->HTMLNodeAttribute(key, value);
}

EMSCRIPTEN_KEEPALIVE
void StaticHTMLNodeEnded(HTMLNodeWalkerInterface* self) {
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

RichFormat::RichFormat() {
  style_.push_back(RichFormat::Style());

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

std::string RichFormat::Format(const char* html, float width) {
  HTMLWalk(html, this);
  return "";
}

void RichFormat::HTMLText(const char* text) {
  if (render_.empty())
    render_.resize(1);

  const int kScale = 64;

  float extra_leading = arimo_->height - (arimo_->ascender - arimo_->descender);
  fprintf(stderr, "Metrics: h: %f a: %f d: %f\n",
          (float)arimo_->height,
          (float)arimo_->ascender, (float)arimo_->descender);
  if (extra_leading < 0) {
    fprintf(stderr, "Metrics: h: %f a: %f d: %f\n",
            (float)arimo_->height,
            (float)arimo_->ascender, (float)arimo_->descender);
    extra_leading = 0.0;
  }
  float full_ascent = extra_leading / 2 + arimo_->ascender;
  float full_descent = extra_leading / 2 + arimo_->descender;

  FT_Set_Char_Size(arimo_, 0, 12 * kScale, 0, 0);

  hb_font_t* hb_font = hb_ft_font_create(arimo_, nullptr);

  hb_font_set_scale(hb_font, 12 * kScale, 12 * kScale);
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
  }
  fprintf(stderr, "Shape: %s%s%s\n",
          style_.back().bold_ ? "bold: " : "",
          style_.back().italic_ ? "italic: " : "",
          text);
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
