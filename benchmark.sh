#!/bin/bash

make clean
make test
./http_server &
mkdir tmp
seq 1 1000 | xargs -n1 -P1000 bash -c 'i=$0; url="http://localhost:8000/index.html"; filename="tmp/${i}.html"; curl -s --http1.1 -o $filename $url'
echo Downloaded $(ls tmp | wc -l) out of 1000
rm -rf tmp

