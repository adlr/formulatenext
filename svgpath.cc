// Copyright

#include "svgpath.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

namespace formulate {

SVGPathIterator::Token SVGPathIterator::Next() {
  while (*str_ == ' ' || *str_ == ',')
    str_++;
  if (*str_ == '\0') {
    return { Token::END, 0 };
  }
  if (*str_ == 'M') {
    str_++;
    return { Token::MOVETO, 0 };
  }
  if (*str_ == 'L') {
    str_++;
    return { Token::LINETO, 0 };
  }
  if (*str_ == 'C') {
    str_++;
    return { Token::CURVETO, 0 };
  }
  char* numend = nullptr;
  errno = 0;
  float val = strtof(str_, &numend);
  if (numend != str_ && errno == 0) {
    // success
    str_ = numend;
    return { Token::NUMBER, val };
  }
  fprintf(stderr, "Seem to have invalid svg str now %c\n", *str_);
  return { Token::END, 0 };
}

}  // namespace formulate
