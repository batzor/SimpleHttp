#!/bin/bash

mkdir tmp
time $(seq 1 10000 | xargs -n1 -P100 bash -c 'i=$0; url="localhost:8080/index.html"; filename="tmp/${i}.html"; wget --quiet -O $filename $url')
echo Downloaded $(ls tmp | wc -l) out of 10000
rm -rf tmp

