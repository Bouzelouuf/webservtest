#include "WebServer.hpp"
#include <iostream>
#include <csignal>
#include <cstdlib>

WebServer* g_server = NULL;

void signalHandler(int signum) {
	(void)signum;
	std::cout << "\nShutting down server..." << std::endl;
	if (g_server)
		g_server->stop();
}

int main(int argc, char** argv) {
	std::string configFile = "config/default.conf";

	if (argc > 1)
		configFile = argv[1];

	try {
		WebServer server(configFile);
		g_server = &server;

		signal(SIGINT, signalHandler);
		signal(SIGTERM, signalHandler);

		server.start();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
