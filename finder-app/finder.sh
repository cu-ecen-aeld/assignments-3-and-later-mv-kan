#!/bin/sh

FILESDIR=$1
SEARCHSTR=$2

if [ $# -ne 2 ]; then 
    echo "you have to define first and second arguments"
    exit 1
fi
if [ ! -d "$FILESDIR" ]; then 
    echo "\"${FILESDIR}\" does not exists"
    exit 1
fi 

LINES_MATCHES=0
FILES_MATCHES=0

for f in "$FILESDIR"/*; do 
    FILES_MATCHES=$(($FILES_MATCHES + 1))
    LINES_MATCHES=$(($(grep -c $SEARCHSTR $f) + $LINES_MATCHES))
done 

echo "The number of files are $FILES_MATCHES and the number of matching lines are $LINES_MATCHES" > /tmp/assignment4-result.txt