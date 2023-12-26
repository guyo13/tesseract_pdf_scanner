#!/bin/bash

BUCKET=$1
KEY=$2

if [ $BUCKET -eq "" ] then
    echo "Missing bucket name"
elif [ $KEY -eq "" ] then
    echo "Missing key"
fi

aws s3api get-object --bucket $BUCKET --key $KEY $KEY \
&& ./search_pdf $KEY codes.txt all `nproc`
