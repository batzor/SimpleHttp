#include "http_server.h"

#include <iostream>

int main(int argc, char* argv[]) {
    SimpleHttp::SimpleServer server;
    server.initSocket(8080);
    server.startServer();
    return 0;
}
