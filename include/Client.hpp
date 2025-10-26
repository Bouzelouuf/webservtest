#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"

class Client {
private:
	int socketFd;
	int serverFd;
	std::string readBuffer;
	std::string writeBuffer;
	size_t writtenBytes;
	HttpRequest request;
	HttpResponse response;
	bool requestComplete;
	bool responseReady;
	time_t lastActivity;

public:
	Client(int fd, int servFd);
	~Client();

	int getSocketFd() const;
	int getServerFd() const;
	time_t getLastActivity() const;
	void updateActivity();

	bool readRequest();
	bool writeResponse();

	void processRequest(const ServerConfig& config);
	bool isRequestComplete() const;
	bool isResponseReady() const;
	bool isWriteComplete() const;

	void close();
};

#endif
