#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <leptonica/allheaders.h>
#include <string>
#include <tesseract/baseapi.h>
#include <vector>

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <path to file> <path to codes>"
                  << std::endl;
        return 1;
    } else if (!std::filesystem::exists(argv[1])) {
        std::cout << "Image File '" << argv[1] << "' does not exist."
                  << std::endl;
        return 1;
    } else if (!std::filesystem::exists(argv[2])) {
        std::cout << "Codes File '" << argv[2] << "' does not exist."
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
    Pix* image = pixRead(argv[1]);
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
