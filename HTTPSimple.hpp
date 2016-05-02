#pragma once
#define HTTP_PORT 80
#include <string>
#include <chrono>

class HTTPSimple
{
	std::string user_agent;
	std::string accept;
	std::string accept_encoding;
	std::string charset;
	std::string accept_language;
	std::string content_type;
	std::string version;
	std::string host;
	std::string page;

	int http_code;

	//std::string referer;
	//std::unordered_map<std::string, std::unordered_map<std::string, std::string>> cookie;
	//bool use_cookies;
	//bool save_cookies;

	void splitUrlIntoPageHost(std::string url);
	std::string buildRequest(std::string type, std::string &data, bool keep_alive = false);
	void removeChunckEncoding(std::string &data);
	void send(std::string &data);

#if __cplusplus >= 199711L
	std::chrono::high_resolution_clock::time_point start_time;
	std::chrono::high_resolution_clock::time_point end_time;
#else
	std::chrono::time_point<std::chrono::monotonic_clock> start_time;
	std::chrono::time_point<std::chrono::monotonic_clock> end_time;
#endif

public:
	HTTPSimple();
	void removeFileFromUrl(std::string &url);
	std::string GETrequest(std::string url, bool keep_alive = false);
	std::string POSTrequest(std::string url, std::string &data, bool keep_alive = false);
	std::string GET(std::string url);
	std::string POST(std::string url, std::string &data);
	int parseResponse(std::string &data);
	int getCode();
	std::string getHost();
	std::string getHost(std::string url);
	int64_t getRequestLatency();
	
};