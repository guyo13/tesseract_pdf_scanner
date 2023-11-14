FROM amazonlinux:2

RUN apk install poppler poppler-dev g++ make tesseract-ocr tesseract-ocr-dev
USER compile
WORKDIR /home/compile
COPY ./Makefile ./search_pdf.cpp ./codes.txt ./
RUN make

CMD ["/bin/sh"]
