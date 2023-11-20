FROM guyor/amazon_linux_tesseract:latest

RUN dnf install -y shadow-utils && adduser -m -s /bin/bash user

USER user

WORKDIR /home/user

COPY ./Makefile ./codes.txt ./

COPY ./src ./src

RUN wget https://github.com/tesseract-ocr/tessdata/blob/4767ea922bcc460e70b87b1d303ebdfed0897da8/eng.traineddata && make

ENV TESSDATA_PREFIX=/home/user

CMD ["/bin/sh"]

