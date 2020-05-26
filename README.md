# SimpleHttp
A HTTP/1.1 server using only Linux system API and C++. My simple benchmarking script showed that it can handle 10k requests
in 53 seconds on my i7-9750H.

## How to run
`$ make test`  
`$ ./http_server`  
`$ ./benchmark.sh`

## Supported methods
`GET` and `HEAD`. All other methods are optional for general-purpose servers according to RFC7231.

## Future works
- Implement persistent connection
- Support other methods and additional headers
