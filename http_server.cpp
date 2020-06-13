#include "http_server.h"

#include <cstring>
#include <sys/ioctl.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

namespace SimpleHttp {
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
#ifdef DEBUG
                DEBUG_ERR("Request too long");
#endif
                return -1;
            }
        }
        if(read_bytes == 0){
            LOG("socket hung up\n");
            return -1;
        }
        clibuf_[read_bytes] = '\0';

#ifdef DEBUG
        DEBUG_ERR("Request %s\n", clibuf_);
#endif


        char *method, *uri, *version;
        method = strtok(clibuf_, " \r");
        uri = strtok(NULL, " \r");
        version = strtok(NULL, " \r");
#ifdef DEBUG
        DEBUG_ERR("method %s uri %s version %s\n",method, uri, version);
#endif

        // load additional header fields
        char *saveptr;
        while(1) {
            char *line = strtok(NULL, "\n");
#ifdef DEBUG
            DEBUG_ERR("line: %s\n", line);
#endif
            if(!line)
                break;
            if(strcmp(line, "\r") && strlen(line) > 0) {
                char *name = strtok_r(line, ":", &saveptr);
                if(strlen(saveptr) == 0) {
#ifdef DEBUG
                    DEBUG_ERR("Bad header: %s\n", name);
#endif
                    sendErrorResponse(sockfd, STATUS_BAD_REQUEST);
                    return -1;
                }
            } 
        }

        if(strcmp(version, HTTP_VERSION)) {
#ifdef DEBUG
            DEBUG_ERR("Bad version: %s\n", version);
#endif
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
                    DEBUG_ERR("Could not open %s\n", string_buf);
                char c;
                int send_len = 0;
                while((c = fgetc(f)) != EOF){
                    write(sockfd, &c, 1);
                    send_len++;
                }
                write(sockfd, CRLF, 2);
                std::cout << "Sent body of length " << send_len << std::endl;
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
        DEBUG_ERR("Response %s\n", string_buf);
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

    SimpleServer::SimpleServer() {
        fds_size_ = 100;
        fd_count_ = 0;
        fds_ = (pollfd *) malloc(sizeof *fds_ * fds_size_);
    }

    void SimpleServer::initSocket(int port){
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        int on = 1;

        if (sockfd < 0) {
            perror("socket() failed");
            exit(-1);
        }

        // set socket to be reuseable
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0) {
            perror("setsockopt(SO_REUSEADDR) failed");
            close(sockfd);
            exit(-1);
        }

        // set socket to be nonblocking
        if (ioctl(sockfd, FIONBIO, (char *)&on) < 0){
            perror("ioctl() failed");
            close(sockfd);
            exit(-1);
        }
        
        struct sockaddr_in serv_addr;
        
        // cleanup garbage values in serv_addr 
        memset(&serv_addr, 0, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        if (bind(sockfd, (sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("binding failed");
            close(sockfd);
            exit(-1);
        }

        if (listen(sockfd, 64) < 0) {
            perror("listen() failed");
            close(sockfd);
            exit(-1);
        }

        // poll for ready-to-read on incoming connection
        fds_[0].fd = sockfd;
        fds_[0].events = POLLIN;
        fd_count_ += 1;
        LOG("Successfully initialized listener.\n");
    }

    // Push back a new file descriptor to the list of file descriptor
    void SimpleServer::addNewFd(int newfd) {
        if (fd_count_ == fds_size_) {
            fds_size_ *= 2;
            fds_ = (struct pollfd *) realloc(fds_, sizeof(struct pollfd) * fds_size_);
        }
        
        fds_[fd_count_].fd = newfd;
        fds_[fd_count_].events = POLLIN;
        
        fd_count_++;
    }

    // Removes file descriptor at the given index
    void SimpleServer::removeFd(int index) {
        // Since we dont need to maintain the order, // we can just rewrite it with the last fd.
        // One of the reason why this is better than just using std::vector
        fds_[index] = fds_[fd_count_ - 1];
        fd_count_ -= 1;
    }

    int SimpleServer::startServer() {
         int newfd;

         bool run_server = true;
         LOG("Starting to poll\n");
         while(run_server) {
             if(poll(fds_, fd_count_, -1) <= 0) {
                 perror("polling failed");
                 exit(-1);
             }
             
             for(int i = 0; i < fd_count_; i++) {
                 if(fds_[i].revents != 0) {
                     if(fds_[i].revents != POLLIN) {
                         DEBUG_ERR("polling returned unexpected result");
                         run_server = false;
                         break;
                     }

                     // if listener is ready to read, handle new connection
                     if(i == 0) {
                         newfd = accept(fds_[0].fd, NULL, NULL);
                         if(newfd < 0) {
                             if(errno != EWOULDBLOCK) {
                                 perror("accept() failed");
                                 run_server = false;
                             }
                             break;
                         }else{
                             addNewFd(newfd);
                             LOG("accepted new connection, total: %d\n", fd_count_);
                         }
                     }else{
                         if(rh_.handleRequest(fds_[i].fd) < 0) {
                             close(fds_[i].fd);
                             removeFd(i);
                         }
                     }
                 }
             }
         }

         // close all the remaining open sockets
         for (int i = 0; i < fd_count_; i++) {
                 if(fds_[i].fd >= 0)
                     close(fds_[i].fd);
                   
         }
    }
}
