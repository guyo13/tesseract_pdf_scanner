#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <leptonica/allheaders.h>
#include <poppler-document.h>
#include <poppler-image.h>
#include <poppler-page-renderer.h>
#include <poppler-page.h>
#include <stdlib.h>
#include <string>
#include <tesseract/baseapi.h>
#include <vector>

int convert_pdf_page(
    std::string& pdf_file, int page_number, std::string& outfile)
{
    std::unique_ptr<poppler::document> doc(
        poppler::document::load_from_file(pdf_file));

    if (!doc) {
        std::cerr << "Failed to open the document." << std::endl;
        return 0;
    }
    int numPages = doc->pages();
    if ( page_number < 1 || page_number > numPages) {
        std::cerr << "Page number " << page_number << " is out of range (1-" << numPages << ")"  << std::endl;
        return 0;
    }
    poppler::page_renderer renderer;
    renderer.set_render_hint(poppler::page_renderer::antialiasing, true);
    renderer.set_render_hint(poppler::page_renderer::text_antialiasing, true);

    // Poppler pages start at index 0
    std::unique_ptr<poppler::page> page(doc->create_page(page_number - 1));
    poppler::image image
        = renderer.render_page(page.get(), 300, 300); // 300 DPI

    if (image.is_valid()) {
        image.save(outfile, "jpeg");
    } else {
        std::cerr << "Failed to render page " << page_number << std::endl;
        return 0;
    }

    return 1;
}

int main(int argc, char** argv)
{
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <path to file> <path to codes> <page num>" << std::endl;
        return 1;
    } else if (!std::filesystem::exists(argv[1])) {
        std::cerr << "Image File '" << argv[1] << "' does not exist."
                  << std::endl;
        return 1;
    } else if (!std::filesystem::exists(argv[2])) {
        std::cerr << "Codes File '" << argv[2] << "' does not exist."
                  << std::endl;
        return 1;
    }
    std::filesystem::path filePath(argv[2]);
    std::vector<std::string> codes;
    std::ifstream file(filePath);

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            codes.push_back(line);
        }
        file.close();
    } else {
        std::cerr << "Unable to open codes file for reading." << std::endl;
        return 1;
    }
    // PDF
    std::string pdf_file(argv[1]);
    // rendered doc
    std::string jpg_file(argv[1]);
    jpg_file += ".jpg";
    if (!convert_pdf_page(pdf_file, atoi(argv[3]), jpg_file)) {
        return 1;
    }

    std::cerr << "Starting OCR on " << jpg_file << std::endl;
    Pix* image = pixRead(jpg_file.c_str());
    tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
    api->Init(NULL, "eng");
    api->SetImage(image);
    api->Recognize(0);
    tesseract::ResultIterator* ri = api->GetIterator();
    tesseract::PageIteratorLevel level = tesseract::RIL_TEXTLINE;
    if (ri != 0) {
        do {
            const char* scanned_line = ri->GetUTF8Text(level);
            for (const std::string& code : codes) {
                const char* found = strstr(scanned_line, code.c_str());
                if (found) {
                    std::cout << "Code '" << code << "' found at position: "
                              << (found - scanned_line) << std::endl;
                    float conf = ri->Confidence(level);
                    int x1, y1, x2, y2;
                    ri->BoundingBox(level, &x1, &y1, &x2, &y2);
                    printf("scanned_line: '%s'; conf: %.2f; BoundingBox: "
                           "%d,%d,%d,%d;\n",
                        scanned_line, conf, x1, y1, x2, y2);
                }
            }
            delete[] scanned_line;
        } while (ri->Next(level));
    }
    delete api;
}
