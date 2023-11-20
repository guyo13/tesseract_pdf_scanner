FROM guyor/amazon_linux_tesseract:latest

RUN dnf install -y shadow-utils && adduser -m -s /bin/bash user

USER user

WORKDIR /home/user

COPY ./Makefile ./codes.txt ./

COPY ./src ./src

RUN make

CMD ["/bin/sh"]

