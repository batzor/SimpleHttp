#include "http_server.h"

#include <cstring>
#include <sstream>
#include <stdio.h>
#include <thread>

namespace SimpleHttp {
    Request::Request(int connfd) {
        this->connfd = connfd;
    }
    int Request::parseHeader() {
        std::stringstream header;

        int nbytes;
        char c;
        const std::string double_crlf = "\r\n\r\n";
        int idx = 0;
        while ((nbytes = read(connfd, &c, 1)) > 0) {
            header << c;

            // read until double CRLF
            if(c == double_crlf[idx]) {
                idx++;
                if(idx == 4)
                    break;
            }else if(c == CR){
                idx = 1;
            }else{
                idx = 0;
            }
        }
        
        std::string s;
#ifdef DEBUG
        getline(header, s, '\n');
        std::cout << "STATUS_LINE:" << s << std::endl;
        header.seekg(0, std::ios::beg);
#endif
        header >> this->method;
        header >> this->uri;
        header >> this->version;
        getline(header, s, '\n');

        // load additional header fields
        while(getline(header, s, '\n')) {
            if(s != "") {
                size_t pos = s.find(':');
                if(pos == std::string::npos) {
                    break;
                }

                std::string name = s.substr(0, pos);
                std::string value = s.substr(pos + 1, s.length());
                headers[name] = value;
            }
        }
        return 0;
    }

    void Request::handleRequest() {
        int status;
        if((status = parseHeader()) < 0) {
#ifdef DEBUG
            std::cout << status << std::endl;
#endif
            Response::sendErrorResponse(connfd, STATUS_BAD_REQUEST);
            return;
        }

        if(this->version != HTTP_VERSION) {
#ifdef DEBUG
            std::cout << this->version << std::endl;
#endif
            Response::sendErrorResponse(connfd, STATUS_HTTP_VERSION_NOT_SUPPORTED);
            return;
        }

        // All general-purpose servers MUST support the methods GET and HEAD.
        // All other methods are OPTIONAL. RFC7231 p21
        if(this->method == "GET" || this->method == "HEAD") {
            std::ifstream f;
            std::string mime_type;
            long file_size;
            if((file_size = UriHandler::getFileByURI(this->uri, f, mime_type)) < 0) {
                Response::sendErrorResponse(connfd, STATUS_NOT_FOUND);
                close(connfd);
                return;
            }

            // additional headers to be filled
            std::map<std::string, std::string> headers;

            // A sender that generates a message containing a payload body SHOULD
            // generate a Content-Type header field in that message unless the
            // intended media type of the enclosed representation is unknown to the
            // sender.

            if(mime_type != "") {
                headers["Content-Type"] = mime_type;
            }
            headers["Content-Length"] = std::to_string(file_size);
            headers["Connection"] = "close";
            Response::sendHeader(connfd, STATUS_OK, headers);

            if(this->method == "GET"){
                char c;
                while(f.get(c))
                    write(connfd, &c, 1);

            }
            close(connfd);
        }
    }

    SimpleServer::SimpleServer(int port, int reuse_address) {
        this->port = port;
        this->reuse_address = 1;
    }

    int SimpleServer::initSocket(){
        this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            DEBUG_ERR("failed to init socket");

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &this->reuse_address, sizeof(int)) < 0)
            DEBUG_ERR("setsockopt(SO_REUSEADDR) failed");
        
        struct sockaddr_in serv_addr;
        
        // cleanup garbage values in serv_addr 
        memset(&serv_addr, 0, sizeof(this->serv_addr));

        this->serv_addr.sin_family = AF_INET;
        this->serv_addr.sin_addr.s_addr = INADDR_ANY;
        this->serv_addr.sin_port = htons(this->port);

        if (bind(sockfd, (struct sockaddr *) &this->serv_addr, sizeof(this->serv_addr)) < 0)
            perror("binding failed");

        if (listen(sockfd, 30000) < 0)
            DEBUG_ERR("listen() failed");

        return sockfd;
    }

    int SimpleServer::startServer() {
         int sockfd = this->initSocket();

         int newsockfd;
         struct sockaddr_in cli_addr;
         socklen_t addr_size = sizeof(cli_addr);

         // Accept incoming connections
         while(true) {
             newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &addr_size);
             if (newsockfd < 0)
                 DEBUG_ERR("failed to accept connection");


             Request req(newsockfd);
             std::thread t(&Request::handleRequest, &req);
             t.detach();
         }
    }
}
