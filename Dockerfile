FROM guyor/amazon_linux_tesseract:latest

RUN dnf install -y shadow-utils && adduser -m -s /bin/bash user

USER user

WORKDIR /home/user

COPY ./Makefile ./search_pdf.cpp ./codes.txt ./

RUN make

CMD ["/bin/sh"]

