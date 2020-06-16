#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "macros.h"
#include "utils.h"

#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <poll.h>
#include <stack>
#include <sys/types.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace SimpleHttp {
    class RequestHandler {
        public:
            RequestHandler();
            bool is_done;
            void sendErrorResponse(int sockfd, int status);
            void sendHeader(int sockfd, int status, std::map<std::string, std::string> &headers);
            int handleRequest(int sockfd);

            int fd_count_;
            // mutex for read/write operation on fds_
            std::mutex rw_mtx;
            // mutex for reallocating fds_
            std::mutex realloc_mtx;

            void addNewFd(int newfd); 
            void removeFd(int index);
            void pollLoop();

        private:
            char clibuf_[1024];
            char string_buf[128];
            std::stack<size_t> remove_list_; 
            int fds_size_;
            struct pollfd *fds_;
    };

    class SimpleServer {
        public:
            SimpleServer();
            bool is_done;
            void initSocket(int port);
            int startServer(void);

        private:
            std::vector<RequestHandler *> handlers_;
            std::vector<std::thread> workers_;
            int listenfd_;
    };
}

#endif
