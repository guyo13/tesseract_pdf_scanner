FROM public.ecr.aws/x9a3w9a8/amazon_linux_tesseract:latest

ARG MAKE_TARGET=build

RUN dnf install -y shadow-utils wget && adduser -m -s /bin/bash user

USER user

WORKDIR /home/user

COPY ./Makefile ./eng.traineddata ./codes.txt ./download_and_run.sh ./

COPY ./src ./src

RUN make $MAKE_TARGET && make clean-objs

ENV TESSDATA_PREFIX=/home/user

ENV OMP_THREAD_LIMIT=1

CMD ["/bin/sh"]

