// Copyright...

#ifndef RICH_FORMAT_H_
#define RICH_FORMAT_H_

#include <string>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <SkCanvas.h>

#include <txt/paragraph_builder.h>

namespace formulate {

// Helper class to parse HTML. For example, the following str would
// trigger the following callbacks:
// 'Hi <b class="cl">There</b><br />Rock&amp;Roll!'
// Text("Hi ");
// NodeStarted("b");
// NodeAttribute("class", "cl");
// Text("There");
// NodeEnded();
// NodeStarted("br");
// NodeEnded();
// Text("Rock&Roll!");

class HTMLNodeWalkerInterface {
 public:
  virtual void HTMLText(const char* text) {}  // not HTML encoded
  virtual void HTMLNodeStarted(const char* tag_name) {}
  virtual void HTMLNodeAttribute(const char* key, const char* value) {}
  virtual void HTMLNodeEnded() {}
};

void HTMLWalk(const char* str, HTMLNodeWalkerInterface* callbacks);

class RichFormat : public HTMLNodeWalkerInterface {
 public:
  RichFormat();

  std::unique_ptr<txt::Paragraph> Format(const char* html);

  void HTMLText(const char* text);
  void HTMLNodeStarted(const char* tag_name);
  void HTMLNodeAttribute(const char* key, const char* value) {}
  void HTMLNodeEnded();

 private:
  std::shared_ptr<txt::FontCollection> font_collection_;
  std::unique_ptr<txt::ParagraphBuilder> paragraph_builder_;
  bool did_first_paragraph_{false};
};

}  // namespace formulate

#endif  // RICH_FORMAT_H_
