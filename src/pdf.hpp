#ifndef OCR_DEV_PDF_HPP
#define OCR_DEV_PDF_HPP
int get_pdf_page_count(std::string& pdf_file);
int convert_pdf_page(
    std::string& pdf_file, int page_number, std::string& outfile);
#endif // OCR_DEV_PDF_HPP
