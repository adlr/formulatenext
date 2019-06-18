// Copyright...

#include <vector>

#include <gtest/gtest.h>

#include "pdfdoc.h"

namespace formulate {

std::vector<char> GenerateSimplePDF(std::vector<const char*> strs) {
  ScopedFPDFDocument doc(FPDF_CreateNewDocument());
  ScopedFPDFPage page(FPDFPage_New(doc.get(),
                                   0, 8.5 * 72, 11 * 72));

  int idx = 0;
  for (const char* str : strs) {
    ScopedFPDFPageObject obj(FPDFPageObj_NewTextObj(
        doc.get(), "Times-Roman", 12.0f));
    std::string message = StrToUTF16LE(str);
    FPDF_WIDESTRING pdf_str =
        reinterpret_cast<FPDF_WIDESTRING>(message.c_str());
    EXPECT_TRUE(FPDFText_SetText(obj.get(), pdf_str));
    FPDFPageObj_Transform(obj.get(), 1, 0, 0, 1, 72, 72 + 20 * idx++);
    FPDFPage_InsertObject(page.get(), obj.release());
  }
  EXPECT_EQ(strs.size(), FPDFPage_CountObjects(page.get()));
  EXPECT_TRUE(FPDFPage_GenerateContent(page.get()));
  EXPECT_EQ(strs.size(), FPDFPage_CountObjects(page.get()));
  FileSaver fs;
  EXPECT_TRUE(FPDF_SaveAsCopy(doc.get(), &fs, FPDF_REMOVE_SECURITY));
  return fs.data_;
}

TEST(PDFDocTest, AppendPagesTest) {
  std::vector<char> a_pdf = GenerateSimplePDF({"A"});
  PDFDoc doc;
  doc.SetLength(a_pdf.size());
  doc.AppendBytes(&a_pdf[0], a_pdf.size());
  doc.FinishLoad();
  EXPECT_EQ(1, doc.Pages());
  doc.AppendPDF(&a_pdf[0], a_pdf.size());
  EXPECT_EQ(2, doc.Pages());
}

struct PDFDocMovePagesTestCase {
  std::vector<std::pair<int, int>> from;
  int to;
  std::vector<int> expected;
};

class PDFDocMovePagesTest :
      public testing::TestWithParam<PDFDocMovePagesTestCase> {
};

TEST_P(PDFDocMovePagesTest, MovePagesTest) {
  PDFDoc doc;
  PDFDocMovePagesTestCase params = GetParam();

  // Set up doc with 5 pages
  std::vector<const char*> strs = {"1", "2", "3", "4", "5"};
  for (int i = 0; i < 5; i++) {
    std::vector<const char*> strs_temp = strs;
    strs_temp.resize(i);
    std::vector<char> pdf = GenerateSimplePDF(strs_temp);
    if (i == 0) {
      doc.SetLength(pdf.size());
      doc.AppendBytes(&pdf[0], pdf.size());
      doc.FinishLoad();
    } else {
      doc.AppendPDF(&pdf[0], pdf.size());
    }
  }
  EXPECT_EQ(5, doc.Pages());
  doc.MovePages(params.from, params.to);
  EXPECT_EQ(5, doc.Pages());
  for (size_t i = 0; i < params.expected.size(); i++) {
    // Get num objects on page
    EXPECT_EQ(params.expected[i], doc.ObjectsOnPage(i)) << "i=" << i;
  }
  doc.undo_manager_.PerformUndo();
  EXPECT_EQ(5, doc.Pages());
  for (int i = 0; i < doc.Pages(); i++) {
    // Get num objects on page
    EXPECT_EQ(i, doc.ObjectsOnPage(i)) << "i=" << i;
  }
}

//
//   0   1   2   3   4   5
//    [0] [1] [2] [3] [4]
//

PDFDocMovePagesTestCase move_pages_test_cases[] = {
  {{{0, 1}}, 0, {0, 1, 2, 3, 4}},
  {{{0, 1}}, 1, {0, 1, 2, 3, 4}},
  {{{0, 1}}, 2, {1, 0, 2, 3, 4}},
  {{{0, 2}}, 1, {0, 1, 2, 3, 4}},
  {{{0, 1}, {3, 4}}, 2, {1, 0, 3, 2, 4}}
};

INSTANTIATE_TEST_CASE_P(PDFDocMovePagesTestParams,
                         PDFDocMovePagesTest,
                         testing::ValuesIn(move_pages_test_cases));

}  // namespace formulate
