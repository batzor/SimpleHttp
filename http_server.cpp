#include "http_server.h"

#include <cstring>
#include <limits.h>
#include <sys/ioctl.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

volatile sig_atomic_t is_done;
void signalHandler(int sig) {
    is_done = true;
}

namespace SimpleHttp {
    RequestHandler::RequestHandler() {
        is_done = false;
        fds_size_ = 100;
        fd_count_ = 0;
        fds_ = (pollfd *) malloc(sizeof *fds_ * fds_size_);
    }

    int RequestHandler::handleRequest(int sockfd) {
        int read_bytes = 0;
        while(read(sockfd, &clibuf_[read_bytes], 1) > 0) {
            if(read_bytes > 4 && clibuf_[read_bytes] == '\n'){
                if(strcmp(clibuf_ + read_bytes - 3, CRLF CRLF) == 0)
                    break;
            }
            read_bytes++;
            if(read_bytes == 1024){
                sendErrorResponse(sockfd, STATUS_NOT_IMPLEMENTED);
                LOG("Request too long\n");
                return -1;
            }
        }
        if(read_bytes == 0){
            return -1;
        }
        clibuf_[read_bytes] = '\0';

        char *method, *uri, *version;
        method = strtok(clibuf_, " \r");
        uri = strtok(NULL, " \r");
        version = strtok(NULL, " \r");

        // load additional header fields
        char *saveptr;
        while(1) {
            char *line = strtok(NULL, "\n");
            if(!line)
                break;
            if(strcmp(line, "\r") && strlen(line) > 0) {
                char *name = strtok_r(line, ":", &saveptr);
                if(strlen(saveptr) == 0) {
                    LOG("Bad header: %s\n", name);
                    sendErrorResponse(sockfd, STATUS_BAD_REQUEST);
                    return -1;
                }
            } 
        }

        if(strcmp(version, HTTP_VERSION)) {
            LOG("Bad version: %s\n", version);
            sendErrorResponse(sockfd, STATUS_HTTP_VERSION_NOT_SUPPORTED);
            return 0;
        }

        // All general-purpose servers MUST support the methods GET and HEAD.
        // All other methods are OPTIONAL. RFC7231 p21
        if(!strcmp(method, "GET") || !strcmp(method, "HEAD")) {
            char *file_name;
            file_name = strtok(uri, "?");
            strcpy(string_buf, SERVE_DIR);
            strcat(string_buf, file_name);

            long file_size = getFileSize(string_buf);
            if(file_size <= 0) {
                sendErrorResponse(sockfd, STATUS_NOT_FOUND);
                return 0;
            }

            std::map<std::string, std::string> headers;
            headers["Content-Length"] = std::to_string(file_size);

            // A sender that generates a message containing a payload body SHOULD
            // generate a Content-Type header field in that message unless the
            // intended media type of the enclosed representation is unknown to the
            // sender.
            std::string mime_type;
            getMimeType(string_buf, mime_type);
            if(mime_type != "") {
                headers["Content-Type"] = mime_type;
            }

            if(!strcmp(method, "GET")){
                FILE *f = fopen(string_buf, "r");
                sendHeader(sockfd, STATUS_OK, headers);
                if(f == NULL)
                    LOG("Could not open %s\n", string_buf);
                char c;
                int send_len = 0;
                while((c = fgetc(f)) != EOF){
                    write(sockfd, &c, 1);
                    send_len++;
                }
                write(sockfd, CRLF, 2);
                fclose(f);
            }else{
                sendHeader(sockfd, STATUS_OK, headers);
            }
            return 0;
        }else{
            sendErrorResponse(sockfd, STATUS_NOT_IMPLEMENTED);
            return -1;
        }
    }

    void RequestHandler::sendErrorResponse(int connfd, int status) {
        std::map<std::string, std::string> headers;
        headers["Content-Length"] = "0";
        headers["Connection"] = "close";
        sendHeader(connfd, status, headers);
    }

    void RequestHandler::sendHeader(int sockfd, int status, std::map<std::string, std::string> &headers) {
        strcpy(string_buf, HTTP_VERSION);
        switch(status) {
            case STATUS_OK:
                strcat(string_buf, " 200 OK" CRLF);
                break;
            case STATUS_BAD_REQUEST:
                strcat(string_buf, " 400 Bad Request" CRLF);
                break;
            case STATUS_NOT_IMPLEMENTED:
                strcat(string_buf, " 501 Not Implemented" CRLF);
                break;
            case STATUS_NOT_FOUND:
                strcat(string_buf, " 404 Not Found" CRLF);
                break;
            case STATUS_HTTP_VERSION_NOT_SUPPORTED:
                strcat(string_buf, " 505 HTTP version not supported" CRLF);
                break;
            default:
                sprintf(string_buf + strlen(string_buf), " %d" CRLF, status);
                break;
        }

#ifdef DEBUG
        LOG("Response %s\n", string_buf);
#endif

        write(sockfd, string_buf, strlen(string_buf));

        bool has_content_length = false;
        std::string line;
        for (auto const& h: headers) {
            if(h.first  == "Content-Length") {
                has_content_length = true;
                continue;
            }

            line = h.first + ": " + h.second + CRLF;
            write(sockfd, line.c_str(), line.length());
        }
        if(has_content_length){
            line = "Content-Length: " + headers["Content-Length"] + CRLF;
            write(sockfd, line.c_str(), line.length());
        }
        write(sockfd, CRLF, 2);
    }

    // Push back a new file descriptor to the list of file descriptor
    void RequestHandler::addNewFd(int newfd) {
        rw_mtx.lock();
        if (fd_count_ == fds_size_) {
            fds_size_ *= 2;
            realloc_mtx.lock();
            fds_ = (struct pollfd *) realloc(fds_, sizeof(struct pollfd) * fds_size_);
            realloc_mtx.unlock();
        }
        
        fds_[fd_count_].fd = newfd;
        fds_[fd_count_].events = POLLIN;
        
        fd_count_++;
        rw_mtx.unlock();
    }

    // Removes file descriptor at the given index
    void RequestHandler::removeFd(int index) {
        // Since we dont need to maintain the order, // we can just rewrite it with the last fd.
        // One of the reason why this is better than just using std::vector
        fds_[index] = fds_[fd_count_ - 1];
        fd_count_ -= 1;
    }

    void RequestHandler::pollLoop() {
         while(!is_done) {
             // if no client to poll, then continue
             if(fd_count_ == 0)
                 continue;

             if(poll(fds_, fd_count_, -1) <= 0) {
                 perror("polling failed");
                 exit(-1);
             }

             // loop through the connected clients
             realloc_mtx.lock();
             for(int i = 0; i < fd_count_; i++) {
                 if(fds_[i].revents != 0) {
                     if(fds_[i].revents != POLLIN) {
                         LOG("polling returned unexpected result");
                         break;
                     }

                     if(handleRequest(fds_[i].fd) < 0) {
                         close(fds_[i].fd);
                         //Removing from the list is unsafe inside the loop
                         remove_list_.push(i);
                     }
                 }
             }
             realloc_mtx.unlock();

             rw_mtx.lock();
             while(!remove_list_.empty()){
                 removeFd(remove_list_.top());
                 remove_list_.pop();
             }
             rw_mtx.unlock();
         }

         // close all the remaining open sockets
         for (int i = 0; i < fd_count_; i++) {
             if(fds_[i].fd >= 0)
                 close(fds_[i].fd);
         }

         free(fds_);
    }

    SimpleServer::SimpleServer() {
        is_done = 0;
        pool_size = std::thread::hardware_concurrency() / 2;
        for(int i = 0;i < pool_size;i++) {
            handlers_.push_back(new RequestHandler);
        }
    }

    void SimpleServer::initSocket(int port){
        listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
        int on = 1;

        if (listenfd_ < 0) {
            perror("socket() failed");
            exit(-1);
        }

        // set socket to be reuseable
        if (setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0) {
            perror("setsockopt(SO_REUSEADDR) failed");
            close(listenfd_);
            exit(-1);
        }

        // set socket to be nonblocking
        if (ioctl(listenfd_, FIONBIO, (char *)&on) < 0){
            perror("ioctl() failed");
            close(listenfd_);
            exit(-1);
        }
        
        struct sockaddr_in serv_addr;
        
        // cleanup garbage values in serv_addr 
        memset(&serv_addr, 0, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        if (bind(listenfd_, (sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("binding failed");
            close(listenfd_);
            exit(-1);
        }

        if (listen(listenfd_, 64) < 0) {
            perror("listen() failed");
            close(listenfd_);
            exit(-1);
        }
        LOG("Successfully initialized listener.\n");
    }

    int SimpleServer::startServer() {
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        LOG("Starting to poll\n");
        // initialize worker threads
        for(auto h: handlers_){
            workers_.push_back(std::thread(&RequestHandler::pollLoop, h));
        }

        int newfd;
        while(!is_done) {
            // if listener is ready to read, handle new connection
            newfd = accept(listenfd_, NULL, NULL);
            if(newfd < 0) {
                if(errno != EWOULDBLOCK) {
                    perror("accept() failed");
                    is_done = true;
                }
            }else{
                int min_thread_work = INT_MAX;
                RequestHandler *free_handler;
                for(int i = 0;i < pool_size;i++) {
                    if(handlers_[i]->fd_count_ < min_thread_work)
                        free_handler = handlers_[i];
                }

                free_handler->addNewFd(newfd);
            }
        }

        cleanupServer();

        return 0;
    }

    void SimpleServer::cleanupServer() {
        for(size_t i = 0;i < pool_size;i++) {
            workers_[i].join();
            delete(handlers_[i]);
            LOG("worker %lu finished successfully.\n", i);
        }
    }
}
