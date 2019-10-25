// Copyright...

#ifndef RICH_FORMAT_H_
#define RICH_FORMAT_H_

#include <string>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

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

// Intermediate layout
//
// These classes represent text that has been laid out. It's an
// intermediate representation between the original HTML and the final
// PDF text object stream. This representation helps determine line
// wrapping and line height.

struct Style {
  bool bold_{false};
  bool italic_{false};
  float size_{12.0};
  void Dump() const {
    fprintf(stderr, "<%s%s %f>", bold_ ? "B" : "", italic_ ? "I" : "", size_);
  }
  // TODO(adlr): font, color, underline
  // For now assume Helvetica, 12, black, no underline
};

// Here, we keep track of all the characters in a row. full_ascent
// is the max of [ascent + 1/2 extra leading] for each char.
// full_descent is max of [descent + 1/2 of extra leading] for each
// char.  Extra leading is defined as: a font's leading (aka line
// height) minus [a font's acent + descent]. If extra leading is
// negative, we treat it as zero.
//
// If a user clicks on the text, we can use this structure to find
// which caret position they are going for.
class LayoutString;
class LayoutStyle;

class LayoutElement {
 public:
  virtual ~LayoutElement() {}
  virtual bool IsLayoutString() { return false; }
  virtual LayoutString* AsLayoutString() { return nullptr; }
  virtual bool IsLayoutStyle() { return false; }
  virtual LayoutStyle* AsLayoutStyle() { return nullptr; }
  virtual void Dump() const {}
};

class LayoutString : public LayoutElement {
 public:
  bool IsLayoutString() { return true; }
  LayoutString* AsLayoutString() { return this; }

  struct LayoutChar {
    float advance_;
    float ideal_advance_;
    uint16_t char_code_;
  };
  void PushChar(LayoutChar layout_char) {
    chars_.push_back(layout_char);
  }
  void Dump() const {
    for (auto lc : chars_) {
      fprintf(stderr, "%c", lc.char_code_);
      if (lc.advance_ != lc.ideal_advance_)
        fprintf(stderr, "/%f/", lc.advance_ - lc.ideal_advance_);
    }
  }

  std::vector<LayoutChar> chars_;
};

class LayoutStyle : public LayoutElement {
 public:
  explicit LayoutStyle(const Style& style) : style_(style) {}
  LayoutStyle* AsLayoutStyle() { return this; }
  bool IsLayoutStyle() { return true; }
  void Dump() const { style_.Dump(); }
    
  Style style_;
};

struct LayoutRow {
  LayoutRow() {}
  LayoutRow(LayoutRow const&) = delete;
  LayoutRow(LayoutRow&& that) noexcept
      : elements_(std::move(that.elements_)) {}
  void operator=(LayoutRow const &unused) = delete;
  void Dump() const {
    fprintf(stderr, "LayoutRow:");
    for (auto it = elements_.begin(); it != elements_.end(); ++it)
      (*it)->Dump();
    fprintf(stderr, "\n");
  }

  std::vector<std::unique_ptr<LayoutElement>> elements_;
};

// RichFormat handles parsing the HTML and generating a sequence of
// LayoutRow objects.

class RichFormat : public HTMLNodeWalkerInterface {
 public:
  RichFormat();

  const std::vector<LayoutRow>& Format(const char* html, float width);

  void HTMLText(const char* text);
  void HTMLNodeStarted(const char* tag_name);
  void HTMLNodeAttribute(const char* key, const char* value) {}
  void HTMLNodeEnded();
  void HandleNewline();
  void HandleStringWithoutNewline(const char* text);

 private:
  FT_Face FT_FaceForStyle(const Style& style);
  static const char* PDFFontForStyle(const Style& style);
  std::pair<float, float> GetFullAscenderAndDescender(const FT_Face face);

  // Look thougth the last row in rows_ and break it (multiple times) if needed.
  // Returns true iff a break was performed.
  bool LineBreakLastRow();

  void InsertCharOntoLastRow(struct LayoutString::LayoutChar layout_char);

  uint16_t CharcodeFromGlyph(FT_Face face, uint16_t glyph_id);

  // Keep a stack of style. Each time a style is changed (e.g. on
  // <b>), we push the new version. When a style is over (e.g. </b>),
  // we pop the latest. The last element (style_.back()) is the
  // current style.
  std::vector<Style> style_;

  // Each row has a LayoutRow object:
  std::vector<LayoutRow> rows_;

  // Width after which to wrap lines
  float width_;

  // TODO(adlr): have a font cache
  FT_Library ftlib_;
  FT_Face arimo_;
};

// Final represention: PDF style

//void CreatePDFTextObjectsFromLayout(...);

}  // namespace formulate

#endif  // RICH_FORMAT_H_
