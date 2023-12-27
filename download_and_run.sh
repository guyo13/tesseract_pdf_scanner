#!/bin/bash

URL=$1
KEY=$2

if [ "$URL" = "" ]; then
    echo "Missing url"
elif [ "$KEY" = "" ]; then
    echo "Missing key"
else
    FILENAME=/tmp/`basename $KEY`
    wget -O $FILENAME $URL && ./search_pdf "$FILENAME" codes.txt all `nproc`
fi

