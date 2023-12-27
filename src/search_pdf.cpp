#include "pdf.hpp"
#include "thirdparty/json.hpp"
#include "util.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <leptonica/allheaders.h>
#include <poppler-document.h>
#include <pthread.h>
#include <string>
#include <tesseract/baseapi.h>
#include <vector>

using json = nlohmann::json;

const char* DOCUMENT_OPEN_FAIL = "Failed to open the document.";
const char* KEYWORDS_OPEN_FAIL = "Unable to open keywords file for reading.";

void panic()
{
    std::cerr << "Memory error\n";
    exit(1);
}

typedef enum WorkerStatus { Running, Success, Fail } WorkerStatus;

class WorkerArgs {
public:
    int worker_index;
    int start_page;
    int end_page;
    std::map<int, json>& results;
    char* keywords_path;
    char* pdf_path;
    WorkerStatus* status;
    WorkerArgs(int workerIndex, int page_number_start, int page_number_end,
        int total_workers, std::map<int, json>& results, char* keywordsPath,
        char* pdfPath, WorkerStatus* status)
        : worker_index(workerIndex)
        , start_page(get_start_page_for_worker(
              workerIndex, page_number_start, page_number_end, total_workers))
        , end_page(get_end_page_for_worker(
              workerIndex, page_number_start, page_number_end, total_workers))
        , results(results)
        , keywords_path(keywordsPath)
        , pdf_path(pdfPath)
        , status(status)
    {
    }
    static int get_start_page_for_worker(
        int worker_index, int start_offset, int end_offset, int total_workers);
    static int get_end_page_for_worker(
        int worker_index, int start_offset, int end_offset, int total_workers);
};
int WorkerArgs::get_start_page_for_worker(
    int worker_index, int start_offset, int end_offset, int total_workers)
{
    int num_jobs_per_worker = (end_offset - start_offset + 1) / total_workers;
    return start_offset + (num_jobs_per_worker * worker_index);
}

int WorkerArgs::get_end_page_for_worker(
    int worker_index, int start_offset, int end_offset, int total_workers)
{
    int is_last = worker_index == total_workers - 1;
    return is_last ? end_offset
                   : (WorkerArgs::get_start_page_for_worker(worker_index + 1,
                          start_offset, end_offset, total_workers)
                       - 1);
}

void process_line(tesseract::ResultIterator& ri,
    tesseract::PageIteratorLevel level, std::vector<std::string>& keywords,
    json& found_keywords)
{
    const char* scanned_line = ri.GetUTF8Text(level);
    for (const std::string& keyword : keywords) {
        const char* found = strstr(scanned_line, keyword.c_str());
        if (found) {
            json bbox = json::object();
            if (!found_keywords.contains(keyword)) {
                found_keywords[keyword] = json::array();
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
            found_keywords[keyword].push_back(bbox);
        }
    }
    delete[] scanned_line;
}

void search_file(std::string& raster_file_path,
    std::vector<std::string>& keywords, json& result)
{
    Pix* image = pixRead(raster_file_path.c_str());
    auto* api = new tesseract::TessBaseAPI();
    api->Init(nullptr, "eng");
    api->SetImage(image);
    api->Recognize(nullptr);
    tesseract::ResultIterator* ri = api->GetIterator();
    tesseract::PageIteratorLevel level = tesseract::RIL_TEXTLINE;
    if (ri != nullptr) {
        json found_keywords = json::object();

        do {
            process_line(*ri, level, keywords, found_keywords);
        } while (ri->Next(level));

        if (!found_keywords.empty()) {
            result["found"] = found_keywords;
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
    std::unique_ptr<poppler::document>& doc, json& result,
    std::vector<std::string>& keywords)
{
    std::string raster_file_path;

    generate_rendered_file_name(base_path, page_number, raster_file_path);

    if (!convert_pdf_page(doc, page_number, raster_file_path)) {
        return 0;
    }

    std::cerr << "Processing " << raster_file_path << " (page number "
              << page_number << ")" << std::endl;

    search_file(raster_file_path, keywords, result);
    result["pageNumber"] = page_number;

    std::remove(raster_file_path.c_str());

    return 1;
}

int load_keywords(char* keyword_file, std::vector<std::string>& keywords)
{
    std::filesystem::path file_path(keyword_file);
    std::ifstream file(file_path);

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            keywords.push_back(line);
        }
        file.close();
        return 1;
    } else {
        return 0;
    }
}

void* worker_process_page(void* _args)
{
    auto* args = (WorkerArgs*)_args;
    WorkerStatus* status = args->status;
    *status = Running;
    std::vector<std::string> keywords;
    std::string pdf_file(args->pdf_path);
    std::unique_ptr<poppler::document> doc(
        (poppler::document::load_from_file(pdf_file)));
    std::cerr << "Worker: " << args->worker_index
              << " started processing pages: " << args->start_page << "-"
              << args->end_page << std::endl;

    if (!load_keywords(args->keywords_path, keywords)) {
        std::cerr << "Worker: " << args->worker_index << " "
                  << KEYWORDS_OPEN_FAIL << std::endl;
        *status = Fail;
        delete args;
        return nullptr;
    } else if (!doc) {
        std::cerr << "Worker: " << args->worker_index << " "
                  << DOCUMENT_OPEN_FAIL << std::endl;
        *status = Fail;
        delete args;
        return nullptr;
    }

    for (int page_number = args->start_page; page_number <= args->end_page;
         page_number++) {
        (args->results)[page_number] = json::object();

        if (!process_page(args->pdf_path, page_number, doc,
                std::ref(args->results[page_number]), keywords)) {
            *status = Fail;
            delete args;
            return nullptr;
        }
    }

    *status = Success;
    delete args;
    return nullptr;
}

int main(int argc, char** argv)
{
    long num_threads = 1;
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <path to file> <path to keywords> <page num>"
                  << std::endl;
        return 1;
    } else if (!std::filesystem::exists(argv[1])) {
        std::cerr << "Image File '" << argv[1] << "' does not exist."
                  << std::endl;
        return 1;
    } else if (!std::filesystem::exists(argv[2])) {
        std::cerr << "Keywords File '" << argv[2] << "' does not exist."
                  << std::endl;
        return 1;
    }
    if (argc >= 5) {
        if (!(num_threads = strtol(argv[4], nullptr, 10))) {
            std::cerr << "Invalid thread count '" << argv[4] << "'."
                      << std::endl;
            return 1;
        }
    }

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
    num_threads = std::min(num_threads, (long)max_page);
    if (max_page < 1
        || !parse_page_range(
            argv[3], page_number_start, page_number_end, max_page)) {
        return 1;
    }

    std::cerr << "Using " << num_threads << " threads to process "
              << page_number_end - page_number_start + 1 << " pages "
              << page_number_start << "-" << page_number_end << ". Doc is "
              << max_page << " pages long." << std::endl;

    std::vector<pthread_t> threads(num_threads, 0);
    std::vector<std::map<int, json> > thread_results;
    auto* statuses = new WorkerStatus[num_threads];
    if (statuses == nullptr) {
        panic();
    }

    for (int i = 0; i < num_threads; i++) {
        thread_results.push_back(std::map<int, json>());
    }

    for (int i = 0; i < num_threads; i++) {
        auto* args = new WorkerArgs(i, page_number_start, page_number_end,
            (int)num_threads, std::ref(thread_results[i]), argv[2], argv[1],
            &statuses[i]);

        pthread_create(&threads[i], nullptr, worker_process_page, args);
    }

    for (auto tid : threads) {
        pthread_join(tid, nullptr);
    }

    json all_pages_result = json::array();
    for (const auto& res : thread_results) {
        for (const auto& page : res) {
            all_pages_result.push_back(page.second);
        }
    }

    int exit_status = 0;
    for (int i = 0; i < num_threads; i++) {
        if (statuses[i] != Success) {
            exit_status = 1;
            std::cerr << "Worker number " << i << " was not successful."
                      << std::endl;
        }
    }

    std::cout << all_pages_result;
    delete[] statuses;

    return exit_status;
}