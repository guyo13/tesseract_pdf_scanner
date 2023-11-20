FROM guyor/amazon_linux_tesseract:latest

RUN dnf install -y shadow-utils && adduser -m -s /bin/bash user

USER user

WORKDIR /home/user

COPY ./Makefile ./eng.traineddata ./codes.txt ./

COPY ./src ./src

RUN make

ENV TESSDATA_PREFIX=/home/user

ENV OMP_THREAD_LIMIT=1

CMD ["/bin/sh"]

