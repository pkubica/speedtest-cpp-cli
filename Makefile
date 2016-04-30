CC=g++
CFLAGS=-std=c++11 -Wall -pedantic -O2 -lpthread

all: speedtest-cpp-cli

parseArguments.o: parseArguments.cpp parseArguments.hpp
	${CC} ${CFLAGS} -c parseArguments.cpp -o parseArguments.o
clientSocket.o: Socket.cpp Socket.hpp
	${CC} ${CFLAGS} -c Socket.cpp -o clientSocket.o
speedtest-cpp-cli.o: clientSocket.o parseArguments.o Source.cpp SharedQueue.hpp
	${CC} ${CFLAGS} -c Source.cpp -o speedtest-cpp-cli.o
speedtest-cpp-cli: speedtest-cpp-cli.o
	${CC} ${CFLAGS} parseArguments.o clientSocket.o speedtest-cpp-cli.o -o speedtest-cpp-cli
