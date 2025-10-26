#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <vector>
#include <map>
#include <poll.h>
#include "Config.hpp"
#include "Server.hpp"
#include "Client.hpp"

class WebServer {
private:
	Config config;
	std::vector<Server> servers;
	std::map<int, Client*> clients;
	std::vector<struct pollfd> pollFds;
	bool running;

	void setupServers();
	void acceptNewConnection(int serverFd);
	void handleClientRead(int clientFd);
	void handleClientWrite(int clientFd);
	void closeClient(int clientFd);
	void checkTimeouts();
	int findServerIndex(int fd);
	const ServerConfig* getServerConfig(int serverFd);

public:
	WebServer();
	WebServer(const std::string& configFile);
	~WebServer();

	void start();
	void stop();
};

#endif
