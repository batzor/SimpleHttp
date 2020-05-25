#include "http_server.h"

#include <iostream>

int main(int argc, char* argv[]) {
    SimpleHttp::SimpleServer server(8000);
    server.startServer();
    return 0;
}
