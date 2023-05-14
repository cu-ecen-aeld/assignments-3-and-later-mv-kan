#!/bin/bash

WRITEFILE=$1
WRITESTR=$2

if [ $# -ne 2 ]; then 
    echo "you have to define first and second arguments"
    exit 1
fi
FILEDIR=$(dirname $WRITEFILE)

mkdir -p $FILEDIR
echo "$WRITESTR" > $WRITEFILE
