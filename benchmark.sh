#!/bin/bash

mkdir tmp
time $(seq 1 100 | xargs -n1 -P1 bash -c 'i=$0; url="localhost:8080/index.html"; filename="tmp/${i}.html"; wget --quiet --no-hsts --no-dns-cache -O $filename $url')
echo Downloaded $(ls tmp | wc -l) out of 100
rm -rf tmp

