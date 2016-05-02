CC=g++
CFLAGS=-std=c++11 -Wall -pedantic -O2 -lpthread

all: speedtest-cpp-cli

parseArguments.o: parseArguments.cpp parseArguments.hpp Exception.hpp
	${CC} ${CFLAGS} -c parseArguments.cpp -o parseArguments.o
HTTPSimple.o: HTTPSimple.cpp HTTPSimple.hpp Exception.hpp
	${CC} ${CFLAGS} -c HTTPSimple.cpp -o HTTPSimple.o
TAGParser.o: TAGParser.cpp TAGParser.hpp
	${CC} ${CFLAGS} -c TAGParser.cpp -o TAGParser.o
clientSocket.o: Socket.cpp Socket.hpp Exception.hpp
	${CC} ${CFLAGS} -c Socket.cpp -o clientSocket.o
speedtest-cpp-cli.o: clientSocket.o parseArguments.o TAGParser.o HTTPSimple.o Source.cpp SharedQueue.hpp SharedVariable.hpp Exception.hpp
	${CC} ${CFLAGS} -c Source.cpp -o speedtest-cpp-cli.o
speedtest-cpp-cli: speedtest-cpp-cli.o
	${CC} ${CFLAGS} parseArguments.o clientSocket.o speedtest-cpp-cli.o TAGParser.o HTTPSimple.o -o speedtest-cpp-cli

clean:
	rm -rf parseArguments.o clientSocket.o speedtest-cpp-cli.o TAGParser.o HTTPSimple.o speedtest-cpp-cli