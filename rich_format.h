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
  virtual void HTMLNodeAttribute(const char* tag_name,
                                 const char* key, const char* value) {}
  virtual void HTMLNodeAttributesEnded(const char* tag_name) {}
  virtual void HTMLNodeEnded() {}
};

void HTMLWalk(const char* str, HTMLNodeWalkerInterface* callbacks);

class RichFormat : public HTMLNodeWalkerInterface {
 public:
  RichFormat();

  std::unique_ptr<txt::Paragraph> Format(const char* html);

  void HTMLText(const char* text);
  void HTMLNodeStarted(const char* tag_name);
  void HTMLNodeAttribute(const char* tag_name,
                         const char* key, const char* value);
  void HTMLNodeAttributesEnded(const char* tag_name);
  void HTMLNodeEnded();

  // Reads value until we complete up to one tag. If found, they are
  // written to out_*. If there may be another tag, a pointer to the
  // next location to look at in value is returned. If nullptr is
  // returned, there are no more tags left.
  const char* ParseStyleOnce(const char* value,
                             std::string* out_tag,
                             std::string* out_tagval);

 private:
  std::shared_ptr<txt::FontCollection> font_collection_;
  std::unique_ptr<txt::ParagraphBuilder> paragraph_builder_;
  bool did_first_paragraph_{false};
  txt::TextStyle new_style_;  // used during starting a new node
};

}  // namespace formulate

#endif  // RICH_FORMAT_H_
