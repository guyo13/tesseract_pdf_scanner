build: pdf.o
	g++ src/search_pdf.cpp pdf.o `pkg-config --libs --static --cflags poppler-cpp lept tesseract libpng libjpeg` -o search_pdf
static: pdf.o
	g++ src/search_pdf.cpp pdf.o -L/usr/local/lib -l:libtesseract.a -l:libleptonica.a `pkg-config --libs --static --cflags poppler-cpp libpng libjpeg` -ltiff -o search_pdf
pdf.o: src/pdf.hpp
	g++ -c src/pdf.cpp `pkg-config --static --cflags poppler-cpp` -o pdf.o
format:
	clang-format -style=file -i src/*.cpp
clean: clean-objs
	rm search_pdf *.o
clean-objs:
	rm *.o