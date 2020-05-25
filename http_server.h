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
    class Request {
        public:
            Request(int connfd);
            void handleRequest(void);
        private:
            int connfd;
            std::string method;
            std::string uri;
            std::string version;
            std::map<std::string, std::string> headers;

            int parseHeader(void);
    };

    class Response {
        public:
            static void sendErrorResponse(int connfd, int status) {
                std::map<std::string, std::string> headers;
                headers["Content-Length"] = "0";
                headers["Connection"] = "close";
                sendHeader(connfd, status, headers);
            }
            static void sendHeader(int connfd, int status, std::map<std::string, std::string> &headers) {
                std::string reason;
                switch(status) {
                    case STATUS_OK:
                        reason = "OK";
                        break;
                    case STATUS_BAD_REQUEST:
                        reason = "Bad Request";
                        break;
                    case STATUS_NOT_IMPLEMENTED:
                        reason = "Not Implemented";
                        break;
                    case STATUS_NOT_FOUND:
                        reason = "Not Found";
                        break;
                    case STATUS_HTTP_VERSION_NOT_SUPPORTED:
                        reason = "HTTP version not supported";
                        break;
                    default:
                        reason = std::to_string(status);
                        break;
                }

#ifdef DEBUG
                std::cout << "Response: " << reason << std::endl;
#endif

                std::string header = HTTP_VERSION + " " + std::to_string(status) + " " + reason + CRLF;
                for (auto const& h: headers) {
                    header = header + h.first + ": " + h.second + CRLF;
                }
                header = header + CRLF;
                write(connfd, header.c_str(), header.length());
            }
    };

    class UriHandler {
        public:
            // returns the file size (-1 in case the file does not exist)
            static long getFileByURI(std::string uri, std::ifstream &f, std::string &mime_type) {
                size_t delim_idx = uri.find('?');
                std::string file_path = SERVE_DIR + uri.substr(0, delim_idx);
 
                int file_size;
                file_size = getFileSize(file_path);
                if(file_size < 0)
                    return file_size;

                getMimeType(file_path, mime_type);
                f.open(file_path);
                return file_size;
            }
        private:
            static void getMimeType(std::string &file_name, std::string &mime_type) {
                if(strEndsWith(file_name, ".html") || strEndsWith(file_name, ".htm"))
                    mime_type = "text/html";
                else if(strEndsWith(file_name, ".css"))
                    mime_type = "text/css";
            }
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
