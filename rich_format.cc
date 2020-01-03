// Copyright...

#include "rich_format.h"

#include <codecvt>
#include <emscripten.h>
#include <locale>

#include <hb.h>
#include <hb-ft.h>
#include <txt/paragraph.h>

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
                             const char* tag_name,
                             const char* key, const char* value) {
  self->HTMLNodeAttribute(tag_name, key, value);
}

EMSCRIPTEN_KEEPALIVE
void StaticHTMLNodeAttributesEnded(HTMLNodeWalkerInterface* self,
                                   const char* tag_name) {
  self->HTMLNodeAttributesEnded(tag_name);
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

RichFormat::RichFormat() {
  font_collection_.reset(new txt::FontCollection());
  font_collection_->SetupDefaultFontManager();
}

std::unique_ptr<txt::Paragraph> RichFormat::Format(const char* html) {
  txt::ParagraphStyle style;
  style.font_family = "Arimo";
  style.font_size = 13.0;
  paragraph_builder_ = txt::ParagraphBuilder::CreateTxtBuilder(
      style, font_collection_);
  txt::TextStyle textstyle;
  textstyle.color = SK_ColorBLACK;
  textstyle.font_families.push_back("Arimo");
  textstyle.font_size = 13.0;
  paragraph_builder_->PushStyle(textstyle);

  did_first_paragraph_ = false;
  HTMLWalk(html, this);
  std::unique_ptr<txt::Paragraph> paragraph(paragraph_builder_->Build());
  paragraph_builder_.reset();
  return paragraph;
}

void RichFormat::HTMLText(const char* text) {
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> utf16conv;
  std::u16string u16text = utf16conv.from_bytes(std::string(text));
  paragraph_builder_->AddText(u16text);
}

void RichFormat::HTMLNodeStarted(const char* tag_name) {
  new_style_ = paragraph_builder_->PeekStyle();

  if (!strcmp(tag_name, "B"))
    new_style_.font_weight = txt::FontWeight::w700;
  if (!strcmp(tag_name, "I"))
    new_style_.font_style = txt::FontStyle::italic;
}

void RichFormat::HTMLNodeAttribute(const char* tag_name,
                                   const char* key, const char* value) {
  if (!strcmp(key, "style")) {
    while (true) {
      std::string tag, tagval;
      const char* rc = ParseStyleOnce(value, &tag, &tagval);
      if (!tag.empty() && !tagval.empty()) {
        if (tag == "font-size") {
          float size = 0;
          int dummy = 0;
          tagval += '0';  // to ensure sscanf success
          if (sscanf(tagval.c_str(), "%fpx%d", &size, &dummy) == 2) {
            new_style_.font_size = size;
          }
        }
      }
      if (!rc)
        break;
      value = rc;
    }
  }
}

void RichFormat::HTMLNodeAttributesEnded(const char* tag_name) {
  paragraph_builder_->PushStyle(new_style_);

  if (!strcmp(tag_name, "P")) {
    if (!did_first_paragraph_)
      did_first_paragraph_ = true;
    else
      HTMLText("\n");
  }
}

void RichFormat::HTMLNodeEnded() {
  paragraph_builder_->Pop();
}

const char* RichFormat::ParseStyleOnce(const char* value,
                                       std::string* out_tag,
                                       std::string* out_tagval) {
  std::string tag, tagval;
  while (*value && isspace(*value))
    value++;
  if (!*value) {
    return nullptr;
  }
  while (*value && !isspace(*value) && *value != ':' && *value != ';') {
    tag += *value;
    value++;
  }
  if (!*value) {
    return nullptr;
  }
  while (*value && isspace(*value)) {
    value++;
  }
  if (!*value || *value != ':')
    return nullptr;
  value++;  // iterate over ':'
  while (*value && isspace(*value)) {
    value++;
  }
  while (*value && *value != ';') {
    tagval += *value;
    value++;
  }
  *out_tag = std::move(tag);
  *out_tagval = std::move(tagval);
  if (*value)  // skip over ';'
    value++;
  if (*value) {
    return value;
  }
  return nullptr;
}

}  // namespace formulate
