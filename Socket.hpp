#pragma once
#include <string>
#include <iostream> //stdout
#include <errno.h>

#ifdef __linux__ 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif // !INVALID_SOCKET

typedef unsigned int SOCKET;

#elif _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")
#else
#error Platform not supported
#endif

class SocketClient
{
public:
	SocketClient(int domain, int type, int protocol, std::string IPaddress, std::string hostname, int port, int timeout = -1);
	void Connect();
	size_t Send(std::string &data);
	size_t Recv(std::string &data, bool read_to_close = false);
	size_t _devNullRecv();
	void setTimeout(int second);
	~SocketClient();


private:
#ifdef _WIN32
	SOCKET m_socSocket = INVALID_SOCKET;
#elif __linux__ 
	int m_socSocket;
#endif
	char m_cBuffer[10240];
	struct hostent *host;
	bool bGetAddrInfoStatus;

	int m_iFamily_domain;
	int m_iSocket_type;
	int m_iProtocol;
	std::string m_sIPaddress;
	std::string m_sHostname;
	int m_iPort;
	int m_iTimeout;
	fd_set m_fdsetSet;
	sockaddr_in m_SockAddr;
};

