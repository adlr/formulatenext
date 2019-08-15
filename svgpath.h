// Copyright

namespace formulate {

class SVGPathIterator {
 public:
  struct Token {
    enum { MOVETO, CURVETO, LINETO, NUMBER, END } type;
    float number;
  };

  explicit SVGPathIterator(const char* str) : str_(str) {}
  Token Next();
 private:
  const char* str_;
};

}  // namespace formulate
