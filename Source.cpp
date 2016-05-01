#if __cplusplus >= 199711L
#if __GXX_EXPERIMENTAL_CXX0X__
#if __linux__
#include <unistd.h>
#include <time.h>
#else

#endif	

#endif

#endif


#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <unordered_map>
#include <fstream>
#include <chrono>
#include <list>
#include <vector>
#include <thread>

#include "parseArguments.hpp"
#include "Socket.hpp"
#include "SharedQueue.hpp"
#include "SharedVariable.hpp"

#define VERSION "0.0.0-1"
#define PROGRAM_NAME "speedtest-cpp-cli"

#define HTTP_PORT 80
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // !M_PI

using namespace std;

bool SpeedTestMeasure;

SharedQueue<size_t> SpeedResults;

SharedVariable<int> SpeedStatus;

string getTagByName(std::string name, std::string &data)
{
	size_t pos = data.find("<" + name + " ");
	if (pos == string::npos)
	{
		return "";
	}
	pos += name.size() + 2;

	size_t end_pos = data.find(">", pos);
	if (end_pos == string::npos)
	{
		return "";
	}

	if (data.at(end_pos - 1) == '/')
	{
		end_pos -= 1;
	}

	if (data.at(end_pos - 1) == ' ')
	{
		end_pos -= 1;
	}

	return data.substr(pos, end_pos - pos);
}




size_t parseTagAttributes(std::string &data, std::unordered_map<std::string, std::string> &attributes, size_t pos = 0)
{
	int state = 0;
	std::string name = "";
	std::string value = "";
	for (; pos < data.size(); ++pos)
	{
		if (state == 0 && data[pos] == '/')
		{
			//cout << "returning" << endl;
			return pos;
		}

		switch (state)
		{
		case 0:
			if (data[pos] == '=')
			{
				++state;
				continue;
			}
			name += data[pos];
			continue;
		case 1:
			if (data[pos] == '"')
			{
				++state;
				continue;
			}
			else
			{
				//TODO: parse error, missing started "
			}
			continue;
		case 2:
			if (data[pos] == '"')
			{
				attributes.insert(pair<string, string>(name, value));
				name = "";
				value = "";
				++state;
				continue;
			}
			value += data[pos];
			continue;
		case 3:
			//cout << "name:" << name << " value:" << value << endl;
			state = 0;
			continue;
		}
	}
	return pos;
}

void splitUrlIntoPageHost(std::string &url, std::string &host)
{
	if (url.compare(0, 7, "http://") == 0)
	{
		url.erase(0, 7);
	}

	unsigned int j = 0;
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
}

string buildHTTPrequest(std::string page, std::string host, bool keepAlive)
{
	std::string user_agent = "Mozilla/5.0 "
#ifdef __linux__ 
		"(Linux; U; 64bit; en-us; Trident/5.0) "
#elif _WIN32
		"(Windows; U; 32bit; en-us) "
#else
#error Platform not supported
#endif
		"(KHTML, like Gecko) "
		PROGRAM_NAME "/"
		VERSION;

	std::string request =
		"GET " + page + " HTTP/1.1\r\n"
		"Accept-Encoding: identity\r\n"
		"Host: " + host + "\r\n"
		"User-Agent: " + user_agent + "\r\n"
		"Connection: close\r\n"
		"\r\n";

	return request;
}

void removeFileFromHTTPAdress(std::string &url)
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

void removeChunckEncoding(std::string &data)
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

int parseHTTPResponse(std::string &data)
{
	string endHeader = "\r\n\r\n";
	size_t pos_end_header = data.find(endHeader);
	std::string HTTPHeader = data.substr(0, pos_end_header);

	int response_code = stoi(HTTPHeader.substr(9, 3));

	if (string::npos != HTTPHeader.find("Transfer-Encoding: chunked"))
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

class SpeedTestClient
{
private:
	int m_constNumber_of_srv_lat;
	int m_constLatence_tries;

	class Server {
	public:
		Server(float distance, std::unordered_map<string, string> &data) : m_fDistance(distance), m_umapData(data)
		{

		}
		float m_fDistance;
		std::unordered_map<string, string> m_umapData;
	};

	std::unordered_map<string, string> m_umapClient;
	std::unordered_map<string, string> m_umapTimes;
	std::unordered_map<string, string> m_umapDownload;
	std::unordered_map<string, string> m_umapUpload;
	std::unordered_map<string, string> m_umapLatency;
	std::unordered_map<string, string> m_umapServerConfig;
	std::unordered_map<int, Server> m_mapServers;
	std::multimap<float, int> m_multimapDistance;

	std::map<string, string> m_mapServerUrls;

	float m_dLat;
	float m_dLon;

	void buildIMGrequests(std::string &path, std::list<std::string> &urls)
	{
		int sizes[10] = { 350, 500, 750, 1000, 1500, 2000, 2500, 3000, 3500, 4000 };
		int count = stoi(m_umapDownload.at("threadsperurl"));
		for (int i = 0; i < 10; ++i)
		{
			for (int j = 0; j < count; ++j)
			{
				urls.push_back(path + "random" + std::to_string((long long int) sizes[i]) + "x" + std::to_string((long long int) sizes[i]) + ".jpg");
			}
		}
	}

	void parseServers(std::string &data)
	{
#ifdef DEBUG
		ofstream response;
		response.open("response.log");
		response.clear();
		response << data;
		response.close();
#endif

		std::string search = "<server ";
		size_t pos = data.find(search, 0);
		if (pos < data.size())
		{
#ifdef DEBUG
			ofstream response;
			response.open("parse.log");
			response.clear();
#endif
#ifdef DEBUG
			cout << "parsing" << endl;
#endif
			while (pos < data.size())
			{
				pos += search.size();
				std::unordered_map<string, string> tmp;
				pos = parseTagAttributes(data, tmp, pos);

				float distance = calculateDistance(to_radian(stof(tmp.at("lat"))), to_radian(stof(tmp.at("lon"))));

				m_mapServers.insert(pair<int, Server>(stoi(tmp.at("id")), Server(distance, tmp)));
				m_multimapDistance.insert(pair<float, int>(distance, stoi(tmp.at("id"))));

#ifdef DEBUG
				response << pos << endl;
#endif

				pos = data.find(search, pos);
			}
#ifdef DEBUG
			cout << "parsed" << endl;
			response.close();
#endif
		}
		else
		{
			//TODO:ERROR PARSING SERVER LIST
		}
	}



public:
	bool checkifServerExists(int id)
	{
		return m_mapServers.find(id) != m_mapServers.end() ? true : false;
	}

	void printClientInfoMessages(std::string state)
	{
		if (state == "clientInfo")
			cout << "Testing from " << m_umapClient.at("isp") << " " << m_umapClient.at("ip") << "..." << endl;
		if (state == "getConfiguration")
			cout << "Retrieving speedtest.net configuration..." << endl;
		if (state == "getServerList")
			cout << "Retrieving speedtest.net server list..." << endl;
	}

	void printServerInfo(int id, int64_t latence)
	{
		if (!checkifServerExists(id))
			return;
		double lat = latence / 1000.0;
		cout << "Choosed server hosted by " << m_mapServers.at(id).m_umapData.at("sponsor") << " (" << m_mapServers.at(id).m_umapData.at("name") << ", " << m_mapServers.at(id).m_umapData.at("country") << ") [" << m_mapServers.at(id).m_fDistance << " km]: " << lat << " ms" << endl;
	}

	void printClientInfo()
	{
		if (!m_umapClient.empty())
		{
			cout << "Testing from " << m_umapClient.at("isp") << " (" << m_umapClient.at("ip") << ")..." << endl;
		}
	}

	void getNearestServerToKM(unsigned int km)
	{
		for (std::multimap<float, int>::iterator it = m_multimapDistance.begin(); it != m_multimapDistance.end(); ++it)
		{
			if (it->first > km) break;
			cout << it->second << ") " << m_mapServers.at(it->second).m_umapData.at("sponsor") << " (" << m_mapServers.at(it->second).m_umapData.at("name") << ", " << m_mapServers.at(it->second).m_umapData.at("country") << ") " << "[" << it->first << " km]" << endl;
		}
	}

	void getListOfServer()
	{
		for (std::multimap<float, int>::iterator it = m_multimapDistance.begin(); it != m_multimapDistance.end(); ++it)
		{
			cout << it->second << ") " << m_mapServers.at(it->second).m_umapData.at("sponsor") << " (" << m_mapServers.at(it->second).m_umapData.at("name") << ", " << m_mapServers.at(it->second).m_umapData.at("country") << ") " << "[" << it->first << " km]" << endl;
		}
	}

	void getConfig()
	{
		SocketClient sockfd(AF_INET, SOCK_STREAM, IPPROTO_TCP, "", "www.speedtest.net", HTTP_PORT);
		std::string data = "";
		try
		{
			sockfd.Connect();
			std::string request = buildHTTPrequest("/speedtest-config.php", "www.speedtest.net", false);
			sockfd.Send(request);
			sockfd.Recv(data, true);
			if (parseHTTPResponse(data) != 200)
			{
				//TODO: exception NOT OK
			}
		}
		catch (exception &e)
		{
			e.what();
		}
#ifdef DEBUG
		cout << "data>" << data << endl;
#endif
		//get TAG: client, times, download, upload, latency
		std::string client = getTagByName("client", data);
		std::string times = getTagByName("times", data);
		std::string download = getTagByName("download", data);
		std::string upload = getTagByName("upload", data);
		std::string latency = getTagByName("latency", data);
		std::string server_config = getTagByName("server-config", data);
		/*
				cout << client << endl;
				cout << times << endl;
				cout << download << endl;
				cout << upload << endl;
				cout << latency << endl;
		*/
		parseTagAttributes(client, m_umapClient);
		parseTagAttributes(times, m_umapTimes);
		parseTagAttributes(download, m_umapDownload);
		parseTagAttributes(upload, m_umapUpload);
		parseTagAttributes(latency, m_umapLatency);
		parseTagAttributes(server_config, m_umapServerConfig);

		//TODO: check IF ALL TAG IS OK

		m_dLat = to_radian(std::stof(m_umapClient.at("lat")));
		m_dLon = to_radian(std::stof(m_umapClient.at("lon")));
	}

	float to_radian(float degrees)
	{
		return (float) (degrees * M_PI) / 180;
	}

	float calculateDistance(float lat, float lon)
	{
		int radius = 6371;

		float u, v, w;
		u = sin((lat - m_dLat) / 2);
		v = sin((lon - m_dLon) / 2);
		w = u * u + cos(m_dLat) * cos(lat) * v * v;
		return (2.0f * radius * atan2(sqrt(w), sqrt(1 - w)));
	}

	void getClosestServers()
	{
		for (std::map<string, string>::iterator it = m_mapServerUrls.begin(); it != m_mapServerUrls.end(); ++it)
		{
			std::string data = "";
			try
			{
				SocketClient sockfd(AF_INET, SOCK_STREAM, IPPROTO_TCP, "", it->first, HTTP_PORT);
				sockfd.Connect();
				std::string request = buildHTTPrequest(it->second, it->first, false);
				sockfd.Send(request);
#ifdef DEBUG
				std::cout << "wait for data from:" << it->first << endl;
#endif
				sockfd.Recv(data, true);
#ifdef DEBUG
				std::cout << "Received:" << it->first << endl;
#endif				
			}
			catch (exception &e)
			{
				cerr << e.what() << endl;
			}

			if (parseHTTPResponse(data) != 200)
			{
#ifdef DEBUG
				cout << data; //DEBUG
#endif
				continue;
			}
			else
			{
				parseServers(data);
				break;
			}
		}

		//TODO: exception, cannot retrieve server list !!!

	}

	int64_t getLatencyByServer(int id)
	{
		if (!checkifServerExists(id)) return 1 * 60 * 1000 * 1000; //minutes
			//TODO EXCEPTION - server doesnt exits

		string url = m_mapServers.at(id).m_umapData.at("url");

		//SERVER URL remove end to slash
		removeFileFromHTTPAdress(url);
		string host = "";
		splitUrlIntoPageHost(url, host);


		std::string request = buildHTTPrequest(url + "latency.txt", host, false);
		std::list<int64_t> request_latence;
		std::string response;
#if __cplusplus >= 199711L
		std::chrono::high_resolution_clock::time_point start_time;
		std::chrono::high_resolution_clock::time_point end_time;
#else
		std::chrono::time_point<std::chrono::monotonic_clock> start_time;
		std::chrono::time_point<std::chrono::monotonic_clock> end_time;
#endif

		for (int i = 0; i < m_constLatence_tries; ++i)
		{
			try
			{
				//CREATE SOCKET
				SocketClient sockfd(AF_INET, SOCK_STREAM, IPPROTO_TCP, "", host, HTTP_PORT);
				//CONNECT
				sockfd.Connect();
				//TIME START
				start_time = chrono::high_resolution_clock::now();

				//SEND req
				sockfd.Send(request);
				//RECEIVE
				sockfd.Recv(response, true);
				end_time = chrono::high_resolution_clock::now();

			}
			catch (exception &e)
			{
				cout << "exception: ";
				cout << e.what();
			}
			// check receieved message -> test=test & 200
			if (parseHTTPResponse(response) == 200)
			{
#ifdef DEBUG
				cout << "data:\"" << response << "\"" << endl;
#endif
				if (response.compare("test=test\n") == 0)
				{
					request_latence.push_back(chrono::duration_cast<chrono::microseconds>(end_time - start_time).count());
#ifdef DEBUG
					cout << "tmp latence" << request_latence.back() << endl;
#endif
					response.clear();
					continue;
				}
			}

			// exception -> 1 minute time
#ifdef DEBUG
			cout << "data:\"" << response << "\"" << endl;
#endif
			request_latence.push_back(1 * 60 * 1000 * 1000); //1 minute
#ifdef DEBUG
			cout << "Hoston we have a problem" << endl;
#endif					//clear for new request

		}
		int64_t sum = 0;
		for (std::list<int64_t>::iterator jt = request_latence.begin(); jt != request_latence.end(); ++jt)
			sum += *jt;

		sum /= request_latence.size();
		return sum;
	}

	int getLatency(int64_t &latence_time)
	{
		std::vector<int64_t> latency;
		std::vector<int> serverID;
		int j = 0;
		for (std::multimap<float, int>::iterator it = m_multimapDistance.begin(); it != m_multimapDistance.end() && j != m_constNumber_of_srv_lat; ++it, ++j)
		{
			//try based on ID
			//NOTE SRVID
			int64_t server_latency;
			server_latency = getLatencyByServer(it->second);
			serverID.push_back(it->second);
			latency.push_back(server_latency);
		}

		//get minimum 
		latence_time = *(latency.begin());
		unsigned int minimum_pos = 0;
		for (unsigned int i = 0; i < latency.size(); ++i)
		{
			if (latence_time > latency.at(i))
			{
				minimum_pos = i;
				latence_time = latency.at(i);
			}
		}

		//get ID server
		//return ID			
		return serverID.at(minimum_pos);
	}

	class urlImage
	{
		std::string m_sUrl;
		std::string m_sHost;
		std::string m_sRequest;
		size_t m_iData_count;


		//int64_t m_i64Timeout;
		//int64_t m_i64Start_time;
		SocketClient *sockfd;

	public:
		//urlImage(std::string &url, int64_t timeout, int64_t start_time): m_sUrl(url), m_i64Timeout(timeout), m_i64Start_time(start_time)
		urlImage(std::string &url) : m_sUrl(url)
		{
			m_iData_count = 0;
			splitUrlIntoPageHost(m_sUrl, m_sHost);
			m_sRequest = buildHTTPrequest(m_sUrl, m_sHost, false);
			sockfd = new SocketClient(AF_INET, SOCK_STREAM, IPPROTO_TCP, "", m_sHost, HTTP_PORT);
			try
			{
				//cout << "connected" << endl;
				//cout << "host:" << m_sHost << "url" << m_sUrl << endl;
			}
			catch (exception &e)
			{
				delete sockfd;
				throw e;
			}
		}

		~urlImage()
		{
			delete sockfd;
		}

		size_t getResult() { return m_iData_count; }
		//std::string getUrl() { return m_sUrl; }
		//int64_t getDuration() { return chrono::duration_cast<chrono::microseconds>(end_time - start_time).count(); }
		//void setDuration(int64_t duration) { m_i64Duration = duration; }
		//void setResult(int data_count) { m_iData_count = data_count; }

		void run()
		{
			try
			{


				sockfd->Connect();
				//check started time
				//cout << "sending request" << endl;
				sockfd->Send(m_sRequest);
				//cout << "req:" << m_sRequest << endl;
				size_t ret;
				while (SpeedTestMeasure && ((ret = sockfd->_devNullRecv()) > 0))
					SpeedResults.add(ret); //SIZE OF BUFFER ON SOCKET -- TODO: TESTING
				return;
			}
			catch (...)
			{
				//data_count is 0 -> result get worst
			}
		}
	};


	class IncomingDataWorker //: public thread // Thread
	{
		int64_t m_i64Duration;
		SharedQueue<urlImage *>& m_sqSourceQueue;
		SharedQueue<urlImage *>& m_sqTargetQueue;

	public:
		IncomingDataWorker(SharedQueue<urlImage *>& source_queue, SharedQueue<urlImage *>& target_queue) : m_sqSourceQueue(source_queue), m_sqTargetQueue(target_queue){}

		void run() {
			for (int i = 0;; i++) {
				//cout << "run is runned" << endl;
				if (m_sqSourceQueue.isThereaWork() > 0)
				{
					if (!SpeedTestMeasure)
					{
						return; //10sec test
					}
					//cout << "want thread get item" << endl;
					urlImage *item = m_sqSourceQueue.soIgetIt();
					//cout << "thread get item" << endl;
					item->run();
					//cout << "thread did item result: " << item->getResult() << endl;
					delete item;
					//m_sqTargetQueue.add(item);
				}
				else
				{
					SpeedStatus.set(SpeedStatus.get()+1);
					return;
				}
			}
		}
	};

	double speedtestDownload(int id)
	{
		if (!checkifServerExists(id))
			return 0;

		int workers_count = stoi(m_umapServerConfig.at("threadcount"));
		if (workers_count == 0)
			return 0; //EXCEPTION !!! 

		std::list<std::string> urls;
		std::string url = m_mapServers.at(id).m_umapData.at("url");
		removeFileFromHTTPAdress(url);

		buildIMGrequests(url, urls);

		SharedQueue<urlImage *> qsource;
		SharedQueue<urlImage *> qtarget;

		std::vector<thread *> workers;
		std::vector<IncomingDataWorker *> workon;
		std::vector<urlImage> images;
		std::list<double> speeds; 

#if __cplusplus >= 199711L
		std::chrono::high_resolution_clock::time_point start_time;
		std::chrono::high_resolution_clock::time_point check_time;
		std::chrono::high_resolution_clock::time_point test_time;
		std::chrono::high_resolution_clock::time_point end_time;
#else
		std::chrono::time_point<std::chrono::monotonic_clock> start_time;
		std::chrono::time_point<std::chrono::monotonic_clock> check_time;
		std::chrono::time_point<std::chrono::monotonic_clock> test_time;
		std::chrono::time_point<std::chrono::monotonic_clock> end_time;
#endif


		SpeedTestMeasure = true;

		start_time = chrono::high_resolution_clock::now();

		for (std::list<std::string>::iterator it = urls.begin(); it != urls.end(); ++it)
		{
			qsource.add(new urlImage(*it));
		}
			
		for (int i = 0; i < workers_count; ++i)
		{
			workon.push_back(new IncomingDataWorker(qsource, qtarget));
		}


		for (int i = 0; i < workers_count; ++i)
		{
			workers.push_back(new thread(&IncomingDataWorker::run, workon.at(i)));
		}


		start_time = chrono::high_resolution_clock::now(); //SET TIMEOUT
		SpeedStatus.set(0);
		size_t datasize;
		int64_t time;
		check_time = chrono::high_resolution_clock::now();
		//cout << "count_images" << count_images << endl;
		while (SpeedStatus.get() != workers_count)
		{
			datasize = 0;
			while (SpeedResults.size() > 0)
			{
				datasize += SpeedResults.remove();
			}

#if __cplusplus >= 199711L
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
#else
#ifdef __linux__
			usleep(200000);
#else
			Sleep(200);
#endif
#endif
			
			if (datasize == 0)
			{
				test_time = chrono::high_resolution_clock::now();
				if (chrono::duration_cast<chrono::seconds>(check_time - start_time).count() > 10)
				{
					break;
				}
			}
				
			end_time = chrono::high_resolution_clock::now();
			time = chrono::duration_cast<chrono::milliseconds>(end_time - check_time).count();
			datasize /= 1000;
			speeds.push_back((double) datasize * 8 / time);
			//cout << "\rDownload speed: " << speeds.back() << " MBit/s";
			cout << ".";
			cout.flush();

			check_time = chrono::high_resolution_clock::now();
			if (chrono::duration_cast<chrono::seconds>(check_time - start_time).count() > 10)
			{
				break;
			}
		}


		double speed_max = 0;
		double speed_sum = 0;
		int speed_count = 0;

		for (size_t i = 0; i < speeds.size(); ++i)
		{
			if (speed_max < speeds.front())
			{
				speed_max = speeds.front();
			}
			speed_sum += speeds.front();
			++speed_count;
			speeds.pop_front();
		}

		double avg_speed = speed_sum / speed_count;
		cout << endl;
		cout << "Maximum download speed: " << speed_max << " MBit/s" << endl;
		cout << "Average download speed: " << avg_speed << " MBit/s" << endl;
		
		SpeedTestMeasure = false;

		for (int i = 0; i < workers_count; ++i)
		{
			workers.at(i)->join();
		}


		while (qtarget.size() > 0)
		{
			urlImage *item = qtarget.remove();
			delete item;
		}

	/*	while (qsource.size() > 0)
		{
			urlImage *item = qsource.remove();
			delete item;
		}*/

	/*	double speed = (double) sum / time;
		double mbps = speed * 8;
		cout << endl;*/
		return avg_speed;

	}

	SpeedTestClient()
	{
		m_constNumber_of_srv_lat = 5;
		m_constLatence_tries = 4;
		m_mapServerUrls.insert(std::pair<string, string>("www.speedtest.net", "/speedtest-servers-static.php"));
		m_mapServerUrls.insert(std::pair<string, string>("c.speedtest.net", "/speedtest-servers-static.php"));
		m_mapServerUrls.insert(std::pair<string, string>("www.speedtest.net", "/speedtest-servers.php"));
		m_mapServerUrls.insert(std::pair<string, string>("c.speedtest.net", "/speedtest-servers.php"));
	}

};


int main(int argc, char *argv [])
{
	const string description =
		"UNDER DEVELOPMENT -> NOT FOR USING!!\n"
		"Internet speed tester using speedtest.net servers writed in C++\n"
		"Author: Petr Kubica GitHub: https://github.com/pkubica/speedtest-cpp-cli\n"
		"Version: " VERSION "\n";

	ParseArguments op(false); //false for using ParseArguments without equation --param value //true --param=value
	op.setDescription(description); //set description of program
	op.setProgramName(PROGRAM_NAME); //set name 'usage program_name:'

	op.addArgument("list", "", "Show available speedtest.net servers sorted by distance for testing", false, false);
	op.addArgument("distance", "", "Display a list of speedtest.net servers within a distance", true, false);
	op.addArgument("server", "", "Specify a server ID", true, false);
	op.addArgument("version", "", "Show the version number and exit", false, false);
	op.addArgument("download", "", "Test only download speed", false, false);
	op.addArgument("upload", "", "Test only upload speed (NOT IMPLEMENTED)", false, false);
	op.addArgument("share", "", "Get url of image about results on speedtest.net (NOT IMPLEMENTED)", false, false);

	try
	{
		op.parseArgs(argc, argv);

	}
	catch (exception &e)
	{
		cout << e.what() << endl << endl;
		op.help();
		return EXIT_FAILURE;
	}

	if (op.checkIfSet("help"))
	{
		op.help();
		return EXIT_SUCCESS;
	}

	if (op.checkIfSet("version"))
	{
		cout << PROGRAM_NAME << " " << VERSION << endl;
		return EXIT_SUCCESS;
	}

	SpeedTestClient client;
	cout << "Getting speedtest.net configuration..." << endl;
	client.getConfig();
	client.printClientInfo();
	cout << "Looking for speedtest.net servers..." << endl;
	client.getClosestServers();

	if (op.checkIfSet("list"))
	{
		client.getListOfServer();
		return EXIT_SUCCESS;
	}

	if (op.checkIfSet("distance"))
	{
		std::string value = op.getParamValue("distance");
		client.getNearestServerToKM(stoi(value));
		return EXIT_SUCCESS;
	}

	int server;
	int64_t latency;

	if (op.checkIfSet("server"))
	{
		if (client.checkifServerExists(stoi(op.getParamValue("server"))))
		{
			server = stoi(op.getParamValue("server"));
			cout << "Checking latency of specified server..." << endl;
			latency = client.getLatencyByServer(server);
		}
		else
		{
			cout << "Server ID " << op.getParamValue("server") << " doesn't exists." << endl;
			return EXIT_FAILURE;
		}
	
	}
	else
	{
		cout << "Getting best speedtest.net server based on latency..." << endl;
		server = client.getLatency(latency);
	}

	client.printServerInfo(server, latency);
	
	if (!op.checkIfSet("upload") || op.checkIfSet("download"))
	{
		cout << "Starting test - download speed";
		double dspeed = client.speedtestDownload(server);
		//cout << "Download speed: " << dspeed << " MBit/s" << endl;
	}

	if (!op.checkIfSet("download") || op.checkIfSet("upload"))
	{
		cout << "Upload speed for now is not implemented" << endl;
	}

	

	return EXIT_SUCCESS;
}