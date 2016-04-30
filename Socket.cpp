#include "Socket.hpp"
#include <fstream>


SocketClient::SocketClient(int domain, int type, int protocol, std::string IPaddress, std::string hostname, int port, int timeout) :
	m_iFamily_domain(domain), m_iSocket_type(type), m_iProtocol(protocol), m_sIPaddress(IPaddress), m_sHostname(hostname), m_iPort(port), m_iTimeout(timeout)
{
	hostent *host = NULL;
	bGetAddrInfoStatus = false;
	
#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cout << "WSAStartup failed.\n"; //DEBUG
		system("pause"); //DEBUG
		return;
	}
#endif

	m_socSocket = socket(m_iFamily_domain, m_iSocket_type, m_iProtocol);

	if (m_socSocket == INVALID_SOCKET)
	{
		std::cout << "Problem INVALID SOCKET" << std::endl;
		throw std::exception(); //TODO: exception cannot create socket
	}

	FD_ZERO(&m_fdsetSet); /* clear the set */
	FD_SET(m_socSocket, &m_fdsetSet);

	if (m_iTimeout != -1)
		setTimeout(m_iTimeout);

	m_SockAddr.sin_family = m_iFamily_domain;

	if (m_sHostname != "")
	{
		host = gethostbyname(m_sHostname.c_str());
		if (host == NULL && m_sIPaddress == "")
		{
			std::cout << "Problem Resolv" << std::endl;
			//TODO: cannot resolv hostname //strerror(errno) -- message
			throw std::exception();
		}
		m_SockAddr.sin_addr.s_addr = *((unsigned long*) host->h_addr);
	}

	if (m_sIPaddress != "" && host == NULL)
	{
		m_SockAddr.sin_addr.s_addr = inet_addr(m_sIPaddress.c_str());
	}
	else
	{
		//TODO: exception not entered IP or hostname
	}

	if (m_iPort != 0)
	{
		m_SockAddr.sin_port = htons(m_iPort);
	}
	else
	{
		//TODO: error INVALID PORT
	}
}

void SocketClient::Connect()
{
	if (connect(m_socSocket, (sockaddr*) (&m_SockAddr), sizeof(m_SockAddr)) != 0) {
		//TODO: exception cannot connect
		std::cout << "Problem Cannot connect" << std::endl;
		throw std::exception();
	}
	//std::cout << "Connected.\n"; //DEBUG
}

size_t SocketClient::Send(std::string &data)
{
	return send(m_socSocket, data.c_str(), (int) data.size(), 0);
}

size_t SocketClient::Recv(std::string &data, bool read_to_close)
{
	if (read_to_close)
	{
		int ret;
		std::memset(&m_cBuffer, 0, sizeof(m_cBuffer));
		while ((ret = recv(m_socSocket, m_cBuffer, sizeof(m_cBuffer)-1, 0)) > 0)
		{
			data.insert(data.size(), m_cBuffer);
			std::memset(&m_cBuffer, 0, sizeof(m_cBuffer));
		}
		return data.size();
	}
	else
	{
		std::memset(&m_cBuffer, 0, sizeof(m_cBuffer));
		int ret = recv(m_socSocket, m_cBuffer, sizeof(m_cBuffer)-1, 0);
		data.assign(m_cBuffer);
		return ret;
	}
}

size_t SocketClient::_devNullRecv()
{
	size_t ret = 0, sum = 0;
	while ((ret = recv(m_socSocket, m_cBuffer, sizeof(m_cBuffer), 0)) > 0)
	{
		sum += ret;
	}	
	return sum;
}

void SocketClient::setTimeout(int second)
{
	struct timeval timeout;
	timeout.tv_sec = second;
	timeout.tv_usec = 500;
	select((SOCKET) m_socSocket, &m_fdsetSet, NULL, NULL, &timeout);
	m_iTimeout = second;
}

SocketClient::~SocketClient()
{
#ifdef _WIN32
	closesocket(m_socSocket);
	WSACleanup();
#elif __linux__ 
	close(m_socSocket);
#endif
}