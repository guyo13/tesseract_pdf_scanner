#!/bin/bash

BUCKET=$1
KEY=$2

if [ "$BUCKET" = "" ]; then
    echo "Missing bucket name"
elif [ "$KEY" = "" ]; then
    echo "Missing key"
else
    aws s3api get-object --bucket "$BUCKET" --key "$KEY" "$KEY" \
    && ./search_pdf "$KEY" codes.txt all `nproc`
fi

