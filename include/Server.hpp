#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include "ServerConfig.hpp"

class Server {
private:
	int socketFd;
	int port;
	std::string host;
	ServerConfig config;

public:
	Server();
	Server(const ServerConfig& conf);
	~Server();

	void setup();
	void close();

	int getSocketFd() const;
	const ServerConfig& getConfig() const;
};

#endif
