#include "thirdparty/json.hpp"
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

using json = nlohmann::json;

const char* DOCUMENT_OPEN_FAIL = "Failed to open the document.";

int get_pdf_page_count(std::string& pdf_file)
{
    std::unique_ptr<poppler::document> doc(
        poppler::document::load_from_file(pdf_file));

    if (!doc) {
        std::cerr << DOCUMENT_OPEN_FAIL << std::endl;
        return -1;
    }

    return doc->pages();
}

int convert_pdf_page(
    std::string& pdf_file, int page_number, std::string& outfile)
{
    std::unique_ptr<poppler::document> doc(
        poppler::document::load_from_file(pdf_file));

    if (!doc) {
        std::cerr << DOCUMENT_OPEN_FAIL << std::endl;
        return 0;
    }

    int numPages = doc->pages();

    if (page_number < 1 || page_number > numPages) {
        std::cerr << "Page number " << page_number << " is out of range (1-"
                  << numPages << ")" << std::endl;
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
    tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
    api->Init(NULL, "eng");
    api->SetImage(image);
    api->Recognize(0);
    tesseract::ResultIterator* ri = api->GetIterator();
    tesseract::PageIteratorLevel level = tesseract::RIL_TEXTLINE;
    if (ri != 0) {
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

int parse_page_range(char* range, int& start, int& stop, int max_page)
{
    std::string rng(range);
    std::string delimiter = "-";
    auto pos = rng.find(delimiter);

    try {
        if (pos == std::string::npos) {
            int num = stoi(rng);
            stop = start = num;
        } else {
            int n1 = stoi(rng.substr(0, pos));
            int n2 = stoi(rng.substr(pos + 1));
            start = n1;
            stop = n2;
        }

        if (start <= 0 || start > max_page) {
            std::cerr << "start position out of range" << std::endl;
            return 0;
        } else if (start > stop) {
            std::cerr << "start position exceeds stop position" << std::endl;
            return 0;
        } else if (stop > max_page) {
            std::cerr << "stop position out of range" << std::endl;
            return 0;
        }
    } catch (std::invalid_argument const& ex) {
        std::cerr << "std::invalid_argument::what(): " << ex.what() << '\n';
        return 0;
    } catch (std::out_of_range const& ex) {
        std::cerr << "std::out_of_range::what(): " << ex.what() << '\n';
        return 0;
    }

    return 1;
}

void generate_rendered_file_name(std::string& name, int page_number)
{
    name += "_";
    name += std::to_string(page_number);
    name += ".jpg";
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
    int page_number_start, page_number_end, max_page;
    page_number_start = page_number_end = 1;
    max_page = get_pdf_page_count(pdf_file);

    if (max_page < 1
        || !parse_page_range(
            argv[3], page_number_start, page_number_end, max_page)) {
        return 1;
    };

    std::cerr << "Starting ocr on '" << argv[1] << "' pages "
              << page_number_start << "-" << page_number_end << std::endl;

    for (int page_number = page_number_start; page_number <= page_number_end;
         page_number++) {
        std::string raster_file_path(argv[1]);
        json result = json::object();

        generate_rendered_file_name(raster_file_path, page_number);

        if (!convert_pdf_page(pdf_file, page_number, raster_file_path)) {
            return 1;
        }

        std::cerr << "Processing " << raster_file_path << " (page number "
                  << page_number << ")" << std::endl;

        search_file(raster_file_path, codes, result);
        result["pageNumber"] = page_number;

        all_pages_result.push_back(result);
    }

    std::cout << all_pages_result;
}
