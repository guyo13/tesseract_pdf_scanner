all: static
build: pdf.o util.o
	g++ src/search_pdf.cpp pdf.o util.o `pkg-config --libs --static --cflags poppler-cpp lept tesseract libpng libjpeg` -o search_pdf
static: pdf.o util.o
	g++ src/search_pdf.cpp pdf.o util.o -L/usr/local/lib -l:libtesseract.a -l:libleptonica.a `pkg-config --libs --static --cflags poppler-cpp libpng libjpeg` -ltiff -o search_pdf
pdf.o: src/pdf.cpp src/pdf.hpp
	g++ -c src/pdf.cpp `pkg-config --static --cflags poppler-cpp` -o pdf.o
util.o: src/util.cpp src/util.h
	g++ -c src/util.cpp -o util.o
clean: clean-objs
	rm search_pdf *.o
clean-objs:
	rm *.o
format:
	clang-format -style=file -i ./src/*.[c,h,cpp,hpp]
