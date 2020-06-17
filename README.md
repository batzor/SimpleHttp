# SimpleHttp
A HTTP/1.1 polling server with thread pooling, using only Linux system API and C++ with the aim of serving as many requests as possible in a short period of time. 

## How to run
`$ make test`  
`$ ./http_server`

## Supported methods
`GET` and `HEAD`. All other methods are optional for general-purpose servers according to RFC7231.

## Future works
- Support other methods and additional headers
- Provide better benchmarking tool
