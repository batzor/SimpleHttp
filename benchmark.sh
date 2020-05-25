#!/bin/bash

make clean
make test
./http_server &
mkdir tmp
time $(seq 1 $1 | xargs -n1 -P$1 bash -c 'i=$0; url="http://localhost:8000/index.html"; filename="tmp/${i}.html"; curl -s --http1.1 -o $filename $url')
echo Downloaded $(ls tmp | wc -l) out of $1
rm -rf tmp

