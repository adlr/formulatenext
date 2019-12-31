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
  txt::TextStyle top = paragraph_builder_->PeekStyle();

  if (!strcmp(tag_name, "B"))
    top.font_weight = txt::FontWeight::w700;
  if (!strcmp(tag_name, "I"))
    top.font_style = txt::FontStyle::italic;

  paragraph_builder_->PushStyle(top);

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

}  // namespace formulate
