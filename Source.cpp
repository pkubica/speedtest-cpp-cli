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
#include "HTTPSimple.hpp"
#include "TAGParser.hpp"
#include "Exception.hpp"

#define VERSION "0.0.0-1"
#define PROGRAM_NAME "speedtest-cpp-cli"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // !M_PI

bool G_bSpeedTestMeasure;

using namespace std;

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

	int m_sServersUrlSize;
	std::string m_sServersUrl[4];

	float m_dLat;
	float m_dLon;

	void buildIMGrequests(std::string &path, std::list<std::string> &urls)
	{
		std::list<int> sizes = { 350, 500, 750, 1000, 1500, 2000, 2500, 3000, 3500, 4000 };
		int count = stoi(m_umapDownload.at("threadsperurl"));
		for (std::list<int>::iterator it = sizes.begin(); it != sizes.end(); ++it)
		{
			for (int j = 0; j < count; ++j)
			{
				urls.push_back(path + "random" + std::to_string((long long int) *it) + "x" + std::to_string((long long int) *it) + ".jpg");
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
				pos = TAGParser::parseTagAttributes(data, tmp, pos);

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
		HTTPSimple http;
		std::string page;
		try
		{
			page = http.GET("http://www.speedtest.net/speedtest-config.php");
			if (http.getCode() != 200)
			{
				//TODO: exception NOT OK
			}
		}
		catch (exception &e)
		{
			cout << e.what() << endl;
		}
#ifdef DEBUG
		cout << "data>" << data << endl;
#endif
		//get TAG: client, times, download, upload, latency
		std::string client = TAGParser::getTagByName("client", page);
		std::string times = TAGParser::getTagByName("times", page);
		std::string download = TAGParser::getTagByName("download", page);
		std::string upload = TAGParser::getTagByName("upload", page);
		std::string latency = TAGParser::getTagByName("latency", page);
		std::string server_config = TAGParser::getTagByName("server-config", page);
		/*
				cout << client << endl;
				cout << times << endl;
				cout << download << endl;
				cout << upload << endl;
				cout << latency << endl;
		*/
		TAGParser::parseTagAttributes(client, m_umapClient);
		TAGParser::parseTagAttributes(times, m_umapTimes);
		TAGParser::parseTagAttributes(download, m_umapDownload);
		TAGParser::parseTagAttributes(upload, m_umapUpload);
		TAGParser::parseTagAttributes(latency, m_umapLatency);
		TAGParser::parseTagAttributes(server_config, m_umapServerConfig);

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
		for (int i = 0; i < m_sServersUrlSize; ++i)
		{
			std::string data = "";
			HTTPSimple http;
			try
			{
				data = http.GET(m_sServersUrl[i]);
			}
			catch (...)
			{
				continue;
			}

			if (http.getCode() != 200)
			{
				continue;
			}
			else
			{
				parseServers(data);
				return;
			}
		}

		throw Exception("Cannot retrieve speedtest server-list");
		//TODO: exception, cannot retrieve server list !!!

	}

	int64_t getLatencyByServer(int id)
	{
		if (!checkifServerExists(id))
			throw Exception("Wrong server ID, cannot test - server doesn't exists");
			// return 1 * 60 * 1000 * 1000; //minutes
			//TODO EXCEPTION - server doesnt exits

		string url = m_mapServers.at(id).m_umapData.at("url");

		//SERVER URL remove end to slash

		std::list<int64_t> request_latency;
		HTTPSimple http;
		http.removeFileFromUrl(url);
		for (int i = 0; i < m_constLatence_tries; ++i)
		{
			std::string data;
			try
			{
				data = http.GET(url + "latency.txt");
			}
			catch (exception &e)
			{
				cout << "exception: ";
				cout << e.what();
			}

			// check receieved message -> test=test & 200
			if (http.getCode() == 200 && data.compare("test=test\n") == 0)
			{
				request_latency.push_back(http.getRequestLatency());
				continue;
			}

			// exception -> 1 minute time
			request_latency.push_back(1 * 60 * 1000 * 1000); //1 minute
			//clear for new request

		}

		int64_t sum = 0;
		for (std::list<int64_t>::iterator jt = request_latency.begin(); jt != request_latency.end(); ++jt)
			sum += *jt;

		sum /= request_latency.size();
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

	class DLJob
	{
		std::string m_sUrl;
		std::string m_sHost;
		std::string m_sRequest;
		int m_iJob_type;
		SharedQueue<size_t> *m_SpeedResults;

		//int64_t m_i64Timeout;
		//int64_t m_i64Start_time;
		SocketClient *sockfd;

	public:
		//urlImage(std::string &url, int64_t timeout, int64_t start_time): m_sUrl(url), m_i64Timeout(timeout), m_i64Start_time(start_time)
		DLJob(std::string &url, std::string &data, int job_type, SharedQueue<size_t> *speedResults):
			m_sUrl(url), m_iJob_type(job_type), m_SpeedResults(speedResults)
		{
			HTTPSimple http;
			if (job_type == 0)
			{
				m_sRequest = http.GETrequest(m_sUrl);
			}
			else
			{
				m_sRequest = http.POSTrequest(m_sUrl, data);
			}
			m_sHost = http.getHost();

			sockfd = new SocketClient(AF_INET, SOCK_STREAM, IPPROTO_TCP, "", m_sHost, HTTP_PORT);
		}

		~DLJob()
		{
			delete sockfd;
		}

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
				if (m_iJob_type == 0)
				{
					sockfd->Send(m_sRequest);
				}
				else
				{
					sockfd->Send(m_sRequest);
				}
				//cout << "req:" << m_sRequest << endl;
				if (m_iJob_type == 0)
				{
					size_t ret;
					while (G_bSpeedTestMeasure && ((ret = sockfd->_devNullRecv()) > 0))
						m_SpeedResults->add(ret); //SIZE OF BUFFER ON SOCKET -- TODO: TESTING
					return;
				}
				else
				{
					size_t ret;
					while (G_bSpeedTestMeasure && ((ret = sockfd->_devNullRecv()) > 0))
						m_SpeedResults->add(ret); //SIZE OF BUFFER ON SOCKET -- TODO: TESTING
					return;
				}

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
		SharedQueue<DLJob *>& m_sqSourceQueue;
		SharedQueue<DLJob *>& m_sqTargetQueue;
		SharedVariable<int> *m_SpeedStatus;
		
	public:
		IncomingDataWorker(SharedQueue<DLJob *>& source_queue, SharedQueue<DLJob *>& target_queue, SharedVariable<int>* speedStatus):
			m_sqSourceQueue(source_queue), m_sqTargetQueue(target_queue), m_SpeedStatus(speedStatus) 
		{
		}

		void run() {
			for (int i = 0;; i++) {
				//cout << "run is runned" << endl;
				if (m_sqSourceQueue.isThereaWork() > 0)
				{
					if (!G_bSpeedTestMeasure)
					{
						return; //10sec test
					}
					//cout << "want thread get item" << endl;
					DLJob *item = m_sqSourceQueue.soIgetIt();
					//cout << "thread get item" << endl;
					item->run();
					//cout << "thread did item result: " << item->getResult() << endl;
					delete item;
					//m_sqTargetQueue.add(item);
				}
				else
				{
					m_SpeedStatus->set(m_SpeedStatus->get()+1);
					return;
				}
			}
		}
	};

	double speedtestDownload(int id)
	{
		if (!checkifServerExists(id))
			throw Exception("Wrong server ID, cannot test - server doesn't exists");

		int workers_count = stoi(m_umapServerConfig.at("threadcount"));
		if (workers_count == 0)
			throw Exception("Configuration of speedtest.net was changed, need to examine");

		std::list<std::string> urls;
		std::string url = m_mapServers.at(id).m_umapData.at("url");
		HTTPSimple http;
		http.removeFileFromUrl(url);

		buildIMGrequests(url, urls);

		SharedQueue<DLJob *> qsource;
		SharedQueue<DLJob *> qtarget;

		SharedQueue<size_t> speedResults;
		SharedVariable<int> speedStatus;

		std::vector<thread *> workers;
		std::vector<IncomingDataWorker *> workon;
		std::vector<DLJob> images;
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


		G_bSpeedTestMeasure = true;

		start_time = chrono::high_resolution_clock::now();
		
		for (std::list<std::string>::iterator it = urls.begin(); it != urls.end(); ++it)
		{
			std::string data = "";
			qsource.add(new DLJob(*it, data, 0, &speedResults));
		}
			
		for (int i = 0; i < workers_count; ++i)
		{
			workon.push_back(new IncomingDataWorker(qsource, qtarget, &speedStatus));
		}


		for (int i = 0; i < workers_count; ++i)
		{
			workers.push_back(new thread(&IncomingDataWorker::run, workon.at(i))); //nejsem si jist
		}

		start_time = chrono::high_resolution_clock::now(); //SET TIMEOUT
		
		speedStatus.set(0);
		size_t datasize;
		int64_t time;
		check_time = chrono::high_resolution_clock::now();
		//cout << "count_images" << count_images << endl;
		while (speedStatus.get() != workers_count)
		{
			datasize = 0;
			while (speedResults.size() > 0)
			{
				datasize += speedResults.remove();
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

		G_bSpeedTestMeasure = false;

		double speed_max = 0;
		double speed_sum = 0;
		int speed_count = 0;

		for (std::list<double>::iterator it = speeds.begin(); it != speeds.end(); ++it)
		{
			if (speed_max < *it)
			{
				speed_max = *it;
			}
			speed_sum += *it;
			++speed_count;
		}

		double avg_speed = speed_sum / speed_count;
		cout << endl;
		cout << "Maximum download speed: " << speed_max << " MBit/s" << endl;
		cout << "Average download speed: " << avg_speed << " MBit/s" << endl;
		
		for (int i = 0; i < workers_count; ++i)
		{
			workers.at(i)->join();
		}

		while (qtarget.size() > 0)
		{
			DLJob *item = qtarget.remove();
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

	void speedTestUpload()
	{
		std::vector<int> sizes = { 250000, 500000 };
		string text = "ABCDEFGHIJKLMNOPQRSTUVWXZabcdefghijklmnopqrstuvwxyz0123456789";
		std::string message1;
		message1.resize(sizes[0]);
		std::string message2;
		message2.resize(sizes[1]);
		int len = text.size();

		for (int i = 0; i < sizes[0]; ++i)
		{
			char ch = text[i % len];
			message2.append(1,ch);
		}

		for (int i = 0; i < sizes[1]; ++i)
		{
			char ch = text[i % len];
			message2.append(1,ch);
		}

		//25x first, 25x second = 50jobs

	}

	SpeedTestClient()
	{
		m_sServersUrlSize = 4;
		m_constNumber_of_srv_lat = 5;
		m_constLatence_tries = 4;
		m_sServersUrl[0] = "http://www.speedtest.net/speedtest-servers-static.php";
		m_sServersUrl[1] = "http://c.speedtest.net/speedtest-servers-static.php";
		m_sServersUrl[2] = "http://www.speedtest.net/speedtest-servers.php";
		m_sServersUrl[3] = "http://c.speedtest.net/speedtest-servers.php";
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