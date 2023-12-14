build:
	g++ src/search_pdf.cpp `pkg-config --libs --static --cflags poppler-cpp lept tesseract libpng libjpeg` -o search_pdf 
static:
	g++ src/search_pdf.cpp -L/usr/local/lib -l:libtesseract.a -l:libleptonica.a `pkg-config --libs --static --cflags poppler-cpp libpng libjpeg` -ltiff -o search_pdf 
format:
	clang-format -style=file -i src/*.cpp
