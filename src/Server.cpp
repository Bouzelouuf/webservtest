#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <iostream>

Server::Server() : socketFd(-1), port(8080), host("0.0.0.0") {}

Server::Server(const ServerConfig& conf) : socketFd(-1), config(conf) {
	port = conf.getPort();
	host = conf.getHost();
}

Server::~Server() {
	close();
}

void Server::setup() {
	socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFd < 0)
		throw std::runtime_error("Failed to create socket");

	int opt = 1;
	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		::close(socketFd);
		throw std::runtime_error("Failed to set socket options");
	}

	int flags = fcntl(socketFd, F_GETFL, 0);
	if (flags < 0 || fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) < 0) {
		::close(socketFd);
		throw std::runtime_error("Failed to set non-blocking mode");
	}

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (host == "0.0.0.0" || host.empty())
		addr.sin_addr.s_addr = INADDR_ANY;
	else
		inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

	if (bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		::close(socketFd);
		throw std::runtime_error("Failed to bind socket");
	}

	if (listen(socketFd, 128) < 0) {
		::close(socketFd);
		throw std::runtime_error("Failed to listen on socket");
	}

	std::cout << "Server listening on " << host << ":" << port << std::endl;
}

void Server::close() {
	if (socketFd >= 0) {
		::close(socketFd);
		socketFd = -1;
	}
}

int Server::getSocketFd() const {
	return socketFd;
}

const ServerConfig& Server::getConfig() const {
	return config;
}
