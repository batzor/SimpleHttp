#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "macros.h"
#include "utils.h"

#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

namespace SimpleHttp {
    class RequestHandler {
        public:
            void sendErrorResponse(int sockfd, int status);
            void sendHeader(int sockfd, int status, std::map<std::string, std::string> &headers);
            int handleRequest(int sockfd);
        private:
            char clibuf_[1024];
            char string_buf[128];
    };

    class SimpleServer {
        public:
            SimpleServer();
            void initSocket(int port);
            int startServer(void);

        private:
            RequestHandler rh_;
            int fd_count_;
            int fds_size_;
            struct pollfd *fds_;

            void addNewFd(int newfd); 
            void removeFd(int index);
    };
}

#endif
