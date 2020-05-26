#!/bin/bash

mkdir tmp
time $(seq 1 10000 | xargs -n1 -P10000 bash -c 'i=$0; url="http://localhost:8000/index.html"; filename="tmp/${i}.html"; curl -s --http1.1 -o $filename $url')
echo Downloaded $(ls tmp | wc -l) out of 10000
rm -rf tmp

