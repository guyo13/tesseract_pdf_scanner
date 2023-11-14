build:
	g++ search_pdf.cpp -o  search_pdf -llept -ltesseract `pkg-config --libs --cflags poppler-cpp`
format:
	clang-format -style=file -i *.cpp
