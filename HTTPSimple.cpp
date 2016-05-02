#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <chrono>
#include "HTTPSimple.hpp"
#include "Socket.hpp"
#include "Exception.hpp"

void HTTPSimple::splitUrlIntoPageHost(std::string url)
{
	if (url.compare(0, 7, "http://") == 0)
	{
		url.erase(0, 7);
	}

	unsigned int j = 0;
	host = "";
	for (; j < url.size() && url[j] != '/'; ++j)
	{
		host += url[j];
	}

	if (j < url.size()) //found /
	{
		url.erase(0, j);
	}
	else //url containts only host address
	{
		url.clear();
		url += '/';
	}
	page = url;
}

std::string HTTPSimple::buildRequest(std::string type, std::string &data, bool keep_alive)
{
	std::string content_option = "";

	std::string request =
		type + " " + page + " " + version + "\r\n"
		"Accept: " + accept + "\r\n"
		"Accept-Encoding: " + accept_encoding + "\r\n"
		"Host: " + host + "\r\n"
		"User-Agent: " + user_agent + "\r\n";

	if (keep_alive)
	{
		request += "Connection: keep-alive\r\n";
	}
	else
	{
		request += "Connection: close\r\n";
	}
			
	if (type == "POST")
	{
		request += "Content-Type: " + content_type + "; charset=" + charset + "\r\n";
		request += "Content-Length: " + std::to_string((long long) data.size());
	}

	/*if (use_cookies)
	{
		if (cookie.find(host) != cookie.end())
		{

			for(std::unordered_map<std::string, std::string>::iterator it = cookie.at(host).begin(); it != cookie.at(host).end(); )
		}
	}*/ //NEXT TODO

	request += "\r\n\r\n";

	if (type == "POST")
	{
		request += data;
	}

	return request;
}

void HTTPSimple::removeChunckEncoding(std::string &data)
{
	std::string tmp = data;
	data.clear();
	int status = 0;
	bool up = false;
	for (std::string::iterator it = tmp.begin(); it != tmp.end(); ++it)
	{
		switch (status)
		{
		case 0:
			if (*it == '\r')
			{
				++status;
				continue;
			}
			data += *it;
			continue;
		case 1:
			if (*it == '\n')
			{
				if (up)
				{
					up = false;
					--status;
					continue;
				}
				else
				{
					up = true;
					++status;
					continue;
				}

			}
			else
			{
				data += '\r';
				data += *it;
				if (up)
				{
					++status;
					continue;
				}
				else
				{
					--status;
					continue;
				}
			}
		case 2:
			if (*it == '\r')
			{
				--status;
			}
			continue;
		}
	}
}


HTTPSimple::HTTPSimple()
{
	//BUILD user_agent for requests
	user_agent = "Mozilla/5.0 "
#ifdef __linux__ 
		"(Linux; U; 64bit; en-us; Trident/5.0) "
#elif _WIN32
		"(Windows; U; 32bit; en-us) "
#else
		"(Unknown; U; 32bit; en-us) "
#endif
		"(KHTML, like Gecko)";

	version = "HTTP/1.1";
	accept_language = "en-US;en;q=0.5";
	//use_cookies = false;
	//save_cookies = false;
	content_type = "text/plain";
	charset = "us-ascii";
	accept = "text/plain";
	accept_encoding = "identity";
	host = "";
}

void HTTPSimple::removeFileFromUrl(std::string &url)
{
#if __cplusplus > 199711L
	while (url.back() != '/')
	{
		url.pop_back();
	}
#else
	while (*url.rbegin() != '/')
	{
		url = url.substr(0, url.size() - 1);
	}
#endif
}

void HTTPSimple::send(std::string &data)
{
	SocketClient sockfd(AF_INET, SOCK_STREAM, IPPROTO_TCP, "", host, HTTP_PORT);
	sockfd.Connect();
	start_time = std::chrono::high_resolution_clock::now();
	sockfd.Send(data);
	data.clear();
	sockfd.Recv(data, true);
	end_time = std::chrono::high_resolution_clock::now();
	return;
}

std::string HTTPSimple::GETrequest(std::string url, bool keep_alive)
{
	splitUrlIntoPageHost(url);
	std::string data = "";
	return buildRequest(std::string("GET"), data, keep_alive);
}

std::string HTTPSimple::GET(std::string url)
{
	splitUrlIntoPageHost(url);
	std::string data = "";
	data = buildRequest(std::string("GET"), data);
	send(data);
	http_code = parseResponse(data);
	return data;
}

std::string HTTPSimple::POSTrequest(std::string url, std::string &data, bool keep_alive)
{
	splitUrlIntoPageHost(url);
	return buildRequest(std::string("POST"), data, keep_alive);
}

std::string HTTPSimple::POST(std::string url, std::string &data)
{
	splitUrlIntoPageHost(url);
	std::string response = buildRequest(std::string("POST"), data);
	send(response);
	http_code = parseResponse(response);
	return response;
}

int HTTPSimple::parseResponse(std::string &data)
{
	std::string endHeader = "\r\n\r\n";

	size_t pos_end_header = data.find(endHeader);
	if (pos_end_header == std::string::npos)
		throw Exception("Cannot parse HTTP header - cannot find end");

	std::string HTTPHeader = data.substr(0, pos_end_header);

	int response_code = stoi(HTTPHeader.substr(9, 3));

	if (std::string::npos != HTTPHeader.find("Transfer-Encoding: chunked"))
	{
		data.erase(0, pos_end_header + 2);
		removeChunckEncoding(data);
	}
	else
	{
		data.erase(0, pos_end_header + 4);
	}

	return response_code;
}

int HTTPSimple::getCode()
{
	return http_code;
}

std::string HTTPSimple::getHost()
{
	return host;
}

std::string HTTPSimple::getHost(std::string url)
{
	splitUrlIntoPageHost(url);
	return host;
}

int64_t HTTPSimple::getRequestLatency()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
}