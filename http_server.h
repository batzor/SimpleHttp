#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "macros.h"
#include "utils.h"

#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

namespace SimpleHttp {
    class ResponseHandler {
        public:
            void sendErrorResponse(int connfd, int status);
            void sendHeader(int connfd, int status, std::map<std::string, std::string> &headers);
    };

    class Request {
        public:
            Request(int connfd);
            void handleRequest(void);
        private:
            ResponseHandler rh_;
            int connfd;
            std::string method;
            std::string uri;
            std::string version;
            std::map<std::string, std::string> headers;

            int parseHeader(void);
    };


    class SimpleServer {
        public:
            int sockfd;
            int port;

            SimpleServer(int port, int reuse_address = 1);
            int initSocket(void);
            int startServer(void);

        private:
            int reuse_address = 1;
            size_t thread_pool_size = 1;
            struct sockaddr_in serv_addr;
    };
}

#endif
