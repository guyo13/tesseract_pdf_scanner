#include <iostream>
#include <string>
#include <cstring>

char const* ALL_PAGES = "all";

int parse_page_range(char* range, int& start, int& stop, int max_page)
{
    std::string rng(range);
    std::string delimiter = "-";
    auto pos = rng.find(delimiter);

    try {
        if (strcmp(ALL_PAGES, rng.c_str()) == 0) {
            start = 1;
            stop = max_page;
        }
        else if (pos == std::string::npos) {
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
