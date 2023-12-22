#!/bin/sh

docker build \
-t $IMAGE_TAG \
--progress=plain . 2>&1
