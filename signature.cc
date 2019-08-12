#include <emscripten.h>

#include <math.h>

#include <opencv2/opencv.hpp>

float sRGBtoLinear(unsigned char val) {
  float ret = val / 255.0f;
  if (ret <= 0.0031308f)
    return ret * 12.92f;
  return 1.055f * pow(ret, 1 / 2.4f) - 0.055f;
}

float RGBtoY(float* vals) {
  return vals[0] *  0.2126  + vals[1] *  0.7152  + vals[2] *  0.0722;
}

void sRGBtoGrey(const cv::Mat& in, cv::Mat* out) {
  for (int r = 0; r < in.rows; r++) {
    for (int c = 0; c < in.cols; c++) {
      const unsigned char* rgb = in.ptr(r, c);
      float vals[3] = {
        sRGBtoLinear(rgb[0]),
        sRGBtoLinear(rgb[1]),
        sRGBtoLinear(rgb[2])
      };
      float* fptr = reinterpret_cast<float*>(out->ptr(r, c));
      *fptr = RGBtoY(vals);
    }
  }
}

cv::Rect MagicSubrect() {
  int rectWidth = 1920 / 6;
  int rectHeight = 1080 / 6;
  int rectLeft = (1920 - rectWidth) / 2;
  int rectTop = (1080 * 2 / 3) - rectHeight;
  return cv::Rect(rectLeft, rectTop, rectWidth, rectHeight);
}

void Sobel(const cv::Mat& in, cv::Mat* out) {
  cv::Mat grad_x, grad_y;
  cv::Mat abs_grad_x, abs_grad_y;
  cv::Sobel(in, grad_x, CV_32F, 1, 0, 3);
  cv::Sobel(in, grad_y, CV_32F, 0, 1, 3);
  out->create(in.rows, in.cols, CV_32F);
  auto xit = grad_x.begin<cv::Vec<float, 1>>();
  auto yit = grad_y.begin<cv::Vec<float, 1>>();
  for (auto oit = out->begin<cv::Vec<float, 1>>();
       oit != out->end<cv::Vec<float, 1>>();
       ++oit) {
    *oit = sqrtf((*xit)[0] * (*xit)[0] +
                 (*yit)[0] * (*yit)[0]);
    ++xit;
    ++yit;
  }
}

float Median(const cv::Mat& grey, const cv::Mat& sob) {
  std::pair<float, float> vals[grey.cols * grey.rows];
  auto sob_it = sob.begin<cv::Vec<float, 1>>();
  int idx = 0;
  for (auto grey_it = grey.begin<cv::Vec<float, 1>>();
       grey_it != grey.end<cv::Vec<float, 1>>();
       ++grey_it) {
    float sobval = (*sob_it)[0];
    if (sobval > 0)
      vals[idx++] = std::make_pair((*grey_it)[0], sobval);
    ++sob_it;
  }
  std::sort(vals, vals + idx, [] (const std::pair<float, float>& left,
                                  const std::pair<float, float>& right) {
              return left.first < right.first;
            });
  int left = 0;
  int right = idx - 1;
  float leftsum = 0.0f;
  float rightsum = 0.0f;
  while (left != right) {
    if (leftsum < rightsum)
      leftsum += vals[left++].second;
    else
      rightsum += vals[right--].second;
  }
  return vals[left].first;
}

constexpr int ConstAbs(int in) {
  return in < 0 ? -in : in;
}

template<int MAXSIZE>
struct PixelTests {
 public:
  // TODO(adlr): make this constexpr. I'm giving up on it at time of writing.
  PixelTests()
      : up_(), down_(), left_(), right_() {
    const float kMaxDist = 20;
    const float kMinThSq = (kMaxDist - 0.5) * (kMaxDist - 0.5);
    const float kMaxThSq = (kMaxDist + 0.5) * (kMaxDist + 0.5);
    for (int y = -kMaxDist; y <= kMaxDist; y++) {
      for (int x = -kMaxDist; x <= kMaxDist; x++) {
        float dsq = y * y + x * x;
        if (dsq > kMinThSq && dsq < kMaxThSq) {
          if (y < 0 && ConstAbs(x) <= ConstAbs(y)) {
            assert(upsz_ <= MAXSIZE);
            up_[upsz_++] = std::make_pair(x, y);
          }
          if (x < 0 && ConstAbs(y) <= ConstAbs(x)) {
            assert(leftsz_ <= MAXSIZE);
            left_[leftsz_++] = std::make_pair(x, y);
          }
          if (y > 0 && ConstAbs(x) <= ConstAbs(y)) {
            assert(downsz_ <= MAXSIZE);
            down_[downsz_++] = std::make_pair(x, y);
          }
          if (x > 0 && ConstAbs(y) <= ConstAbs(x)) {
            assert(rightsz_ <= MAXSIZE);
            right_[rightsz_++] = std::make_pair(x, y);
          }
        }
      }
    }
  }
  std::pair<int, int> up_[MAXSIZE];
  std::pair<int, int> down_[MAXSIZE];
  std::pair<int, int> left_[MAXSIZE];
  std::pair<int, int> right_[MAXSIZE];
  int upsz_{0};
  int downsz_{0};
  int leftsz_{0};
  int rightsz_{0};
};

class ComponentAnalizer {
 public:
  explicit ComponentAnalizer(const cv::Mat& thresh)
      : thresh_(&thresh) {
    ids_ = cv::Mat(thresh.rows, thresh.cols, CV_32SC1);
    ids_.setTo(cv::Scalar(0));  // set all to 0
  }
  int Width() const { return ids_.cols; }
  int Height() const { return ids_.rows; }

  int ComponentAtPixel(int xpos, int ypos) {
    if (ids_.at<int>(ypos, xpos) > 0)
      return ids_.at<int>(ypos, xpos);
    if (thresh_->at<float>(ypos, xpos) != 0)
      return -1;  // pixel isn't part of a component
    int id = next_id_++;
    Flood(xpos, ypos, id);
    return id;
  }

  std::set<int> ReachableComponents(const cv::Rect rect) {
    std::set<int> ret;
    const int bottom = rect.y + rect.height;
    const int right = rect.x + rect.width;
    for (int y = rect.y; y < bottom; y++) {
      for (int x = rect.x; x < right; x++) {
        int comp = ComponentAtPixel(x, y);
        if (comp < 0)
          continue;
        if (ret.find(comp) != ret.end())
          continue;
        if (!ComponentIsValid(comp)) {
          fprintf(stderr, "found invalid component in subrect %d\n", comp);
          ret.clear();
          return ret;
        }
        ret.insert(comp);
      }
    }
    if (ret.empty()) {
      fprintf(stderr, "no components found\n");
      return ret;
    }
    // expand w/ reachable valid componets
    std::set<int> todo = ret;
    std::set<int> invalid;
    while (!todo.empty()) {
      std::set<int> found;
      for (auto it : todo) {
        std::set<int> reachable = Nearby(it);
        for (auto reached : reachable) {
          if (invalid.find(reached) != invalid.end())
            continue;
          if (ret.find(reached) == ret.end() &&
              found.find(reached) == found.end()) {
            if (!ComponentIsValid(reached)) {
              invalid.insert(reached);
              continue;
            }
            found.insert(reached);
          }
        }
      }
      ret.insert(found.begin(), found.end());
      todo = std::move(found);
    }
    return ret;
  }

  void ImageWithComponents(const std::set<int>& ids, cv::Mat* out) const {
    *out = cv::Mat(thresh_->rows, thresh_->cols, thresh_->type());
    for (int y = 0; y < Height(); y++) {
      for (int x = 0; x < Width(); x++) {
        if (ids.find(ids_.at<int>(y, x)) != ids.end()) {
          out->at<float>(y, x) = 0;
        } else {
          out->at<float>(y, x) = 1;
        }
      }
    }
  }

 private:
  // Put found hit components into out
  void HitComponents(const std::pair<int, int>* tests,
                              int tests_len,
                              int xpos, int ypos,
                              std::set<int>* out) {
    for (int i = 0; i < tests_len; i++) {
      int xi = xpos + tests[i].first;
      int yi = ypos + tests[i].second;
      if (xi >= 0 && yi >= 0 && xi < Width() && yi < Height()) {
        int id = ComponentAtPixel(xi, yi);
        if (id > 0)
          out->insert(id);
      }
    }
  }

  std::set<int> Nearby(int id) {
    std::set<int> ret;
    for (int y = 0; y < Height(); y++) {
      for (int x = 0; x < Width(); x++) {
        if (ComponentAtPixel(x, y) != id)
          continue;
        if (x > 0 && ids_.at<int>(y, x - 1) == 0)
          HitComponents(tests_.left_, tests_.leftsz_, x, y, &ret);
        if (y > 0 && ids_.at<int>(y - 1, x) == 0)
          HitComponents(tests_.up_, tests_.upsz_, x, y, &ret);
        if (x < (Width() - 1) && ids_.at<int>(y, x + 1) == 0)
          HitComponents(tests_.right_, tests_.rightsz_, x, y, &ret);
        if (y < (Height() - 1) && ids_.at<int>(y + 1, x) == 0)
          HitComponents(tests_.down_, tests_.downsz_, x, y, &ret);
      }
    }
    ret.erase(id);
    return ret;
  }

  void Flood(int xpos, int ypos, int id) {
    bool didUp = false;
    bool didDown = false;
    std::vector<std::pair<int, int>> todo;
    todo.push_back(std::make_pair(xpos, ypos));
    while (!todo.empty()) {
      xpos = todo.back().first;
      ypos = todo.back().second;
      todo.pop_back();
      // rewind current line as much as possible
      while (xpos >= 0 && thresh_->at<float>(ypos, xpos) == 0)
        xpos--;
      xpos++;
      didUp = didDown = false;
      // Scan across the whole line, pushing work into todo if we see any to do
      while (xpos < Width() && thresh_->at<float>(ypos, xpos) == 0) {
        ids_.at<int>(ypos, xpos) = id;
        if (!didUp && ypos > 0 &&
            thresh_->at<float>(ypos - 1, xpos) == 0 &&
            ids_.at<int>(ypos - 1, xpos) == 0) {
          todo.push_back(std::make_pair(xpos, ypos - 1));
          didUp = true;
        } else if (didUp && ypos > 0 &&
                   thresh_->at<float>(ypos - 1, xpos) != 0) {
          didUp = false;
        }
        if (!didDown && ypos < (Height() - 1)  &&
            thresh_->at<float>(ypos + 1, xpos) == 0 &&
            ids_.at<int>(ypos + 1, xpos) == 0) {
          todo.push_back(std::make_pair(xpos, ypos + 1));
          didDown = true;
        } else if (didDown && ypos < (Height() - 1) &&
                   thresh_->at<float>(ypos + 1, xpos) != 0) {
          didDown = false;
        }
        xpos++;
      }
    }
  }

  bool ComponentIsValid(int id) {
    // TODO(adlr): Use a circle, not square, to test
    const int kInvalidSize = 20;
    for (int y = 0; y < Height() - kInvalidSize; y++) {
      for (int x = 0; x < Width() - kInvalidSize; x++) {
        // Sanity-check two corners before doing exhaustive search
        if (ids_.at<int>(y, x) != id ||
            ids_.at<int>(y + kInvalidSize - 1, x + kInvalidSize - 1) != id)
          continue;
        // Do exhaustive search
        bool valid = false;
        for (int i = y; i < y + kInvalidSize; i++) {
          for (int j = x; j < x + kInvalidSize; j++) {
            if (ids_.at<int>(i, j) != id) {
              valid = true;
              goto next;
            }
          }
        }
        if (!valid)
          return false;
     next:
        int unused = 0;  // need a statement for the label
      }
    }
    return true;
  }

  const cv::Mat* thresh_;
  cv::Mat ids_;
  int next_id_{1};
  const PixelTests<30> tests_;
};

void Crop(cv::Mat in, cv::Mat* out, int border) {
  int left = 0;
  int top = 0;
  int right = in.cols;
  int bottom = in.rows;
  for ( ; top < bottom; top++) {
    for (int x = left; x < right; x++) {
      if (in.at<float>(top, x) == 0)
        goto bottom;
    }
  }
bottom:
  for ( ; bottom > top; bottom--) {
    for (int x = left; x < right; x++) {
      if (in.at<float>(bottom - 1, x) == 0)
        goto left;
    }
  }
left:
  for ( ; left < right; left++) {
    for (int y = top; y < bottom; y++) {
      if (in.at<float>(y, left) == 0)
        goto right;
    }
  }
right:
  for (; right > left; right--) {
    for (int y = top; y < bottom; y++) {
      if (in.at<float>(y, right - 1) == 0)
        goto final;
    }
  }
final:
  if (left == right || top == bottom)
    return;
  left = std::max(0, left - border);
  top = std::max(0, top - border);
  right = std::min(in.cols, right + border);
  bottom = std::min(in.rows, bottom + border);
  *out = cv::Mat(in, cv::Rect(left, top, right - left, bottom - top));
}

void Show(const cv::Mat& mat) {
  int step = mat.step;
  EM_ASM_({
      bridge_showGrey($0, $1, $2, $3);
    }, mat.ptr(0, 0), mat.cols, mat.rows, step);
}

void ShowFinal(const cv::Mat& mat) {
  int step = mat.step;
  EM_ASM_({
      bridge_renderBitmap($0, $1, $2, $3);
    }, mat.ptr(0, 0), mat.cols, mat.rows, step);
}

extern "C" {

EMSCRIPTEN_KEEPALIVE
void ProcessImage(char* bytes, int width, int height) {
  cv::Mat thresh(height, width, CV_8UC1);
  {
    cv::Mat mat(height, width, CV_8UC4, bytes);
    cv::Mat grey(height, width, CV_32FC1);
    sRGBtoGrey(mat, &grey);

    // {
    //   // blur beginning
    //   cv::Mat gb;
    //   cv::GaussianBlur(grey, gb, cv::Size(3, 3), 1);
    //   grey = gb;
    // }

    // unsharp mask
    float amt = 1.5f;
    cv::Mat gb;
    cv::GaussianBlur(grey, gb, cv::Size(3, 3), 1);
    try {
      grey *= 1 + amt;
      gb *= amt;
      cv::Mat sharp = grey - gb;
      // Show(sharp);

      cv::Mat filtered;
      cv::bilateralFilter(sharp, filtered, 3, 10, 10);
      // cv::Mat filtered = grey;
      cv::Mat smallGrey(filtered, MagicSubrect());
      // Show(smallGrey);
      cv::Mat sob;
      Sobel(smallGrey, &sob);
      float med = Median(smallGrey, sob);
      cv::threshold(filtered, thresh, med, 1, cv::THRESH_BINARY);
    } catch( cv::Exception& e ) {
      const char* err_msg = e.what();
      fprintf(stderr, "ex: %s\n", err_msg);
      return;
    }
  }
  // Show(thresh);
  ComponentAnalizer ca(thresh);
  std::set<int> ids = ca.ReachableComponents(MagicSubrect());
  fprintf(stderr, "found %zu comps\n", ids.size());
  if (ids.empty()) {
    cv::Mat debug(thresh, MagicSubrect());
    Show(debug);
    return;
  }
  cv::Mat sig;
  ca.ImageWithComponents(ids, &sig);
  cv::Mat cropped;
  Crop(sig, &cropped, 10);
  // Show(cropped);
  ShowFinal(cropped);
}

}  // extern "C
