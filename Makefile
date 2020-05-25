CPP=g++
OUTPUT=http_server

all: build

build: main.cpp http_server.cpp
	${CPP} -o ${OUTPUT} -pthread -DDEBUG main.cpp http_server.cpp

test: main.cpp http_server.cpp
	${CPP} -o ${OUTPUT} -pthread main.cpp http_server.cpp
clean:
	rm http-server
