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
        char c[4];
        int len = 0;
        while((nbytes = read(this->connfd, &c[0], 1)) > 0) {
            header << c[0];
            len++;
            if(c[0] == LF && len >= 4) {
                header.seekg(-4, header.end);
                header.get(c[0]);
                header.get(c[1]);
                header.get(c[2]);
                header.get(c[3]);
                if(strcmp(c, CRLF CRLF) == 0)
                    break;
            }
        }

        header.seekg(0, header.beg);

        std::string s;
        header >> this->method;
        header >> this->uri;
        header >> this->version;
#ifdef DEBUG
        std::cout << "status: " << this->uri << " " << this->version << "eol"<< std::endl;
#endif
        getline(header, s, '\n');
        // load additional header fields
        while(getline(header, s, '\n')) {
            if(s != "\r" && s != "") {
                size_t pos = s.find(':');
                if(pos == std::string::npos) {
#ifdef DEBUG
                    std::cout << "bad header" << s << s.length() << std::endl;
#endif
                    return -1;
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
            rh_.sendErrorResponse(this->connfd, STATUS_BAD_REQUEST);
            return;
        }

        if(this->version != HTTP_VERSION) {
#ifdef DEBUG
            std::cout << "Bad version:" << this->version << std::endl;
#endif
            rh_.sendErrorResponse(this->connfd, STATUS_HTTP_VERSION_NOT_SUPPORTED);
            close(this->connfd);
            return;
        }

        // All general-purpose servers MUST support the methods GET and HEAD.
        // All other methods are OPTIONAL. RFC7231 p21
        if(this->method == "GET" || this->method == "HEAD") {
            std::ifstream f;
            std::string file_path;
            getFilePathFromUri(this->uri, file_path);
            long file_size = getFileSize(file_path);
            if(file_size < 0) {
                rh_.sendErrorResponse(this->connfd, STATUS_NOT_FOUND);
                close(this->connfd);
                return;
            }

            headers["Content-Length"] = std::to_string(file_size);

            // additional headers to be filled
            std::map<std::string, std::string> headers;

            // A sender that generates a message containing a payload body SHOULD
            // generate a Content-Type header field in that message unless the
            // intended media type of the enclosed representation is unknown to the
            // sender.
            std::string mime_type;
            getMimeType(file_path, mime_type);
            if(mime_type != "") {
                headers["Content-Type"] = mime_type;
            }
            headers["Connection"] = "close";
            rh_.sendHeader(this->connfd, STATUS_OK, headers);

            if(this->method == "GET"){
                f.open(file_path);
                char c;
                while(f.get(c))
                    write(this->connfd, &c, 1);

                f.close();
            }
            close(this->connfd);
        }else{
            rh_.sendErrorResponse(this->connfd, STATUS_NOT_IMPLEMENTED);
            close(this->connfd);
        }
    }

    void ResponseHandler::sendErrorResponse(int connfd, int status) {
        std::map<std::string, std::string> headers;
        headers["Content-Length"] = "0";
        headers["Connection"] = "close";
        sendHeader(connfd, status, headers);
    }

    void ResponseHandler::sendHeader(int connfd, int status, std::map<std::string, std::string> &headers) {
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


        std::string line = HTTP_VERSION + " " + std::to_string(status) + " " + reason + CRLF;
        write(connfd, line.c_str(), line.length());

        bool has_content_length = false;
        for (auto const& h: headers) {
            if(h.first  == "Content-Length") {
                has_content_length = true;
                continue;
            }

            line = h.first + ": " + h.second + CRLF;
            write(connfd, line.c_str(), line.length());
        }
        if(has_content_length){
            line = "Content-Length: " + headers["Content-Length"] + CRLF;
            write(connfd, line.c_str(), line.length());
        }
        write(connfd, CRLF, 2);
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

        if (listen(sockfd, 100000) < 0)
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


             Request *req = new Request(newsockfd);
             std::thread t(&Request::handleRequest, req);
             t.detach();
         }
    }
}
