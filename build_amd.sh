#!/bin/sh

docker build \
-t $IMAGE_TAG \
--progress=plain --build-arg="MAKE_TARGET=$MAKE_TARGET". 2>&1
