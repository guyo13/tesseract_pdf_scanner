#ifndef OCR_DEV_PDF_HPP
#define OCR_DEV_PDF_HPP
#include <memory>
#include <poppler-document.h>
#include <string>

int convert_pdf_page(std::unique_ptr<poppler::document>& doc, int page_number,
    std::string& outfile);
#endif // OCR_DEV_PDF_HPP
