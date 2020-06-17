# SimpleHttp
A HTTP/1.1 polling server with thread pooling, using only Linux system API and C++.

# Performance
Used i5 for server and client (2.5GHzx4), that communicates over school intranet.
1. Multithreading @ 031bec. avg time to serve 10k request is 11.189ms
2. Polling @ master. avg time to serve 10k request is 12.772ms
3. Polling with threadpool @ feature/threadpool. avg time to serve 10k request is 12.090ms
All using the benchmark script in master branch.

## How to run
`$ make test`  
`$ ./http_server`

## Supported methods
`GET` and `HEAD`. All other methods are optional for general-purpose servers according to RFC7231.

## Future works
- Support other methods and additional headers
- Provide better benchmarking tool
