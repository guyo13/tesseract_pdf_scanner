#include <string>
#include <iostream>
#include <poppler-document.h>
#include <poppler-image.h>
#include <poppler-page-renderer.h>
#include <poppler-page.h>

int get_pdf_page_count(std::string& pdf_file)
{
    std::unique_ptr<poppler::document> doc(
        (poppler::document::load_from_file(pdf_file)));

    if (!doc) {
        return -1;
    }

    return doc->pages();
}

int convert_pdf_page(
    std::string& pdf_file, int page_number, std::string& outfile)
{
    std::unique_ptr<poppler::document> doc(
        (poppler::document::load_from_file(pdf_file)));

    if (!doc) {
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