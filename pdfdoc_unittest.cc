// Copyright...

#include <vector>

#include <gtest/gtest.h>

#include "pdfdoc.h"

namespace formulate {

std::vector<char> GenerateSimplePDF(const char* str) {
  ScopedFPDFDocument doc(FPDF_CreateNewDocument());
  ScopedFPDFPage page(FPDFPage_New(doc.get(),
                                   0, 8.5 * 72, 11 * 72));
  ScopedFPDFPageObject obj(FPDFPageObj_NewTextObj(
      doc.get(), "Times-Roman", 12.0f));

  std::string message = StrToUTF16LE(str);
  FPDF_WIDESTRING pdf_str = reinterpret_cast<FPDF_WIDESTRING>(message.c_str());
  EXPECT_TRUE(FPDFText_SetText(obj.get(), pdf_str));
  FPDFPageObj_Transform(obj.get(), 1, 0, 0, 1, 72, 72);
  FPDFPage_InsertObject(page.get(), obj.release());
  EXPECT_TRUE(FPDFPage_GenerateContent(page.get()));
  FileSaver fs;
  EXPECT_TRUE(FPDF_SaveAsCopy(doc.get(), &fs, FPDF_REMOVE_SECURITY));
  return fs.data_;
}

TEST(PDFDocTest, AppendPagesTest) {
  std::vector<char> a_pdf = GenerateSimplePDF("A");
  PDFDoc doc;
  doc.SetLength(a_pdf.size());
  doc.AppendBytes(&a_pdf[0], a_pdf.size());
  doc.FinishLoad();
  EXPECT_EQ(1, doc.Pages());
  doc.AppendPDF(&a_pdf[0], a_pdf.size());
  EXPECT_EQ(2, doc.Pages());
}

}  // namespace formulate
