#include "pdf.hpp"
#include "thirdparty/json.hpp"
#include "util.h"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <leptonica/allheaders.h>
#include <poppler-document.h>
#include <string>
#include <tesseract/baseapi.h>
#include <vector>

using json = nlohmann::json;

const char* DOCUMENT_OPEN_FAIL = "Failed to open the document.";

void process_line(tesseract::ResultIterator& ri,
    tesseract::PageIteratorLevel level, std::vector<std::string>& codes,
    json& found_codes)
{
    const char* scanned_line = ri.GetUTF8Text(level);
    for (const std::string& code : codes) {
        const char* found = strstr(scanned_line, code.c_str());
        if (found) {
            json bbox = json::object();
            if (!found_codes.contains(code)) {
                found_codes[code] = json::array();
            }
            int x1, y1, x2, y2;
            ri.BoundingBox(level, &x1, &y1, &x2, &y2);
            bbox["startPos"] = found - scanned_line;
            bbox["confidence"] = ri.Confidence(level);
            bbox["xStart"] = x1;
            bbox["xEnd"] = x2;
            bbox["yStart"] = y1;
            bbox["yEnd"] = y2;
            bbox["text"] = std::string(scanned_line);
            found_codes[code].push_back(bbox);
        }
    }
    delete[] scanned_line;
}

void search_file(std::string& raster_file_path, std::vector<std::string>& codes,
    json& result)
{
    Pix* image = pixRead(raster_file_path.c_str());
    auto* api = new tesseract::TessBaseAPI();
    api->Init(nullptr, "eng");
    api->SetImage(image);
    api->Recognize(nullptr);
    tesseract::ResultIterator* ri = api->GetIterator();
    tesseract::PageIteratorLevel level = tesseract::RIL_TEXTLINE;
    if (ri != nullptr) {
        json found_codes = json::object();

        do {
            process_line(*ri, level, codes, found_codes);
        } while (ri->Next(level));

        if (!found_codes.empty()) {
            result["found"] = found_codes;
        }
    }
    delete api;
}

void generate_rendered_file_name(
    char* base_path, int page_number, std::string& name)
{
    name += base_path;
    name += "_";
    name += std::to_string(page_number);
    name += ".jpg";
}

int process_page(char* base_path, int page_number,
    std::unique_ptr<poppler::document>& doc, json& all_pages_result,
    std::vector<std::string>& codes)
{
    std::string raster_file_path;
    json result = json::object();

    generate_rendered_file_name(base_path, page_number, raster_file_path);

    if (!convert_pdf_page(doc, page_number, raster_file_path)) {
        return 0;
    }

    std::cerr << "Processing " << raster_file_path << " (page number "
              << page_number << ")" << std::endl;

    search_file(raster_file_path, codes, result);
    result["pageNumber"] = page_number;

    all_pages_result.push_back(result);
    std::remove(raster_file_path.c_str());

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

    json all_pages_result = json::array();
    std::string pdf_file(argv[1]);
    std::unique_ptr<poppler::document> doc(
        (poppler::document::load_from_file(pdf_file)));
    if (!doc) {
        std::cerr << DOCUMENT_OPEN_FAIL << std::endl;
        return 1;
    }

    int page_number_start, page_number_end, max_page;
    page_number_start = page_number_end = 1;
    max_page = doc->pages();

    if (max_page < 1
        || !parse_page_range(
            argv[3], page_number_start, page_number_end, max_page)) {
        return 1;
    }

    std::cerr << "Starting ocr on '" << argv[1] << "' pages "
              << page_number_start << "-" << page_number_end << std::endl;

    for (int page_number = page_number_start; page_number <= page_number_end;
         page_number++) {
        if (!process_page(argv[1], page_number, doc, all_pages_result, codes)) {
            return 1;
        }
    }

    std::cout << all_pages_result;
}