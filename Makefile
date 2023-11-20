build:
	g++ `pkg-config --libs --cflags poppler-cpp lept tesseract libpng libjpeg` src/search_pdf.cpp -o search_pdf 
format:
	clang-format -style=file -i *.cpp
