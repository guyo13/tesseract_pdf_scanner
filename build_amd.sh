#!/bin/sh

docker build \
-t $IMAGE_TAG \
--progress=plain --build-args="MAKE_TARGET=$MAKE_TARGET". 2>&1
