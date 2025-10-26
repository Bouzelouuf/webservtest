#include "WebServer.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <ctime>
#include <stdexcept>

#define CLIENT_TIMEOUT 60

WebServer::WebServer() : running(false) {}

WebServer::WebServer(const std::string& configFile) : running(false) {
	config.loadConfig(configFile);
	setupServers();
}

WebServer::~WebServer() {
	stop();
}

void WebServer::setupServers() {
	const std::vector<ServerConfig>& serverConfigs = config.getServers();

	for (size_t i = 0; i < serverConfigs.size(); i++) {
		Server server(serverConfigs[i]);
		server.setup();
		servers.push_back(server);

		struct pollfd pfd;
		pfd.fd = server.getSocketFd();
		pfd.events = POLLIN;
		pfd.revents = 0;
		pollFds.push_back(pfd);
	}
}

void WebServer::start()
{
		if (servers.empty())
			throw std::runtime_error("No servers configured");

		running = true;
		std::cout << "WebServer started" << std::endl;

		int loop_count = 0;
		while (running) {
			loop_count++;
			if (loop_count % 1000 == 0) {
				std::cout << "Loop iteration: " << loop_count << std::endl;
			}

			int pollCount = poll(&pollFds[0], pollFds.size(), 1000);

			if (pollCount < 0) {
				if (running)
					std::cerr << "Poll error" << std::endl;
				break;
			}

        std::cout << "Poll returned: " << pollCount << std::endl;


		for (size_t i = 0; i < pollFds.size() && pollCount > 0; i++) {
			if (pollFds[i].revents == 0)
				continue;

			pollCount--;

			int serverIdx = findServerIndex(pollFds[i].fd);
			if (serverIdx >= 0) {
				if (pollFds[i].revents & POLLIN)
					acceptNewConnection(pollFds[i].fd);
			} else {
				if (pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
					closeClient(pollFds[i].fd);
				} else if (pollFds[i].revents & POLLIN) {
					handleClientRead(pollFds[i].fd);
				} else if (pollFds[i].revents & POLLOUT) {
					handleClientWrite(pollFds[i].fd);
				}
			}
		}

		checkTimeouts();
	}

	std::cout << "WebServer stopped" << std::endl;
}

void WebServer::stop() {
	running = false;

	for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
		delete it->second;
	}
	clients.clear();

	for (size_t i = 0; i < servers.size(); i++) {
		servers[i].close();
	}
	servers.clear();
	pollFds.clear();
}

void WebServer::acceptNewConnection(int serverFd) {
	int clientFd = accept(serverFd, NULL, NULL);
	if (clientFd < 0)
		return;

	int flags = fcntl(clientFd, F_GETFL, 0);
	if (flags < 0 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) < 0) {
		::close(clientFd);
		return;
	}

	Client* client = new Client(clientFd, serverFd);
	clients[clientFd] = client;

	struct pollfd pfd;
	pfd.fd = clientFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	pollFds.push_back(pfd);

	std::cout << "New client connected: " << clientFd << std::endl;
}

void WebServer::handleClientRead(int clientFd) {
	std::map<int, Client*>::iterator it = clients.find(clientFd);
	if (it == clients.end())
		return;

	Client* client = it->second;

	if (!client->readRequest()) {
		closeClient(clientFd);
		return;
	}

	if (client->isRequestComplete()) {
		const ServerConfig* serverConfig = getServerConfig(client->getServerFd());
		if (serverConfig) {
			client->processRequest(*serverConfig);

			for (size_t i = 0; i < pollFds.size(); i++) {
				if (pollFds[i].fd == clientFd) {
					pollFds[i].events = POLLOUT;
					break;
				}
			}
		}
	}
}

void WebServer::handleClientWrite(int clientFd) {
	std::map<int, Client*>::iterator it = clients.find(clientFd);
	if (it == clients.end())
		return;

	Client* client = it->second;

	if (!client->writeResponse()) {
		closeClient(clientFd);
		return;
	}

	if (client->isWriteComplete()) {
		closeClient(clientFd);
	}
}

void WebServer::closeClient(int clientFd) {
	std::map<int, Client*>::iterator it = clients.find(clientFd);
	if (it != clients.end()) {
		delete it->second;
		clients.erase(it);
	}

	for (std::vector<struct pollfd>::iterator pfd = pollFds.begin(); pfd != pollFds.end(); ++pfd) {
		if (pfd->fd == clientFd) {
			pollFds.erase(pfd);
			break;
		}
	}

	std::cout << "Client disconnected: " << clientFd << std::endl;
}

void WebServer::checkTimeouts() {
	time_t now = std::time(NULL);
	std::vector<int> toClose;

	for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
		if (now - it->second->getLastActivity() > CLIENT_TIMEOUT) {
			toClose.push_back(it->first);
		}
	}

	for (size_t i = 0; i < toClose.size(); i++) {
		std::cout << "Client timeout: " << toClose[i] << std::endl;
		closeClient(toClose[i]);
	}
}

int WebServer::findServerIndex(int fd) {
	for (size_t i = 0; i < servers.size(); i++) {
		if (servers[i].getSocketFd() == fd)
			return i;
	}
	return -1;
}

const ServerConfig* WebServer::getServerConfig(int serverFd) {
	for (size_t i = 0; i < servers.size(); i++) {
		if (servers[i].getSocketFd() == serverFd)
			return &servers[i].getConfig();
	}
	return NULL;
}
