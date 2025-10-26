#include "Config.hpp"
#include "Utils.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

Config::Config() {}

Config::Config(const std::string& filename) {
	loadConfig(filename);
}

Config::~Config() {}

void Config::loadConfig(const std::string& filename) {
	configFile = filename;
	parseConfigFile(filename);
}

const std::vector<ServerConfig>& Config::getServers() const {
	return servers;
}

std::string Config::trim(const std::string& str) {
	return Utils::trim(str);
}

void Config::parseConfigFile(const std::string& filename) {
	std::ifstream file(filename.c_str());
	if (!file.is_open())
		throw std::runtime_error("Failed to open config file: " + filename);

	std::string line;
	while (std::getline(file, line)) {
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue;

		if (line == "server {") {
			parseServer(file, line);
		}
	}

	if (servers.empty())
		throw std::runtime_error("No server configuration found");
}

void Config::parseServer(std::ifstream& file, std::string& line) {
	ServerConfig server;
	LocationConfig* currentLocation = NULL;
	bool inLocation = false;

	while (std::getline(file, line)) {
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue;

		if (line == "}") {
			if (inLocation) {
				inLocation = false;
				currentLocation = NULL;
			} else {
				servers.push_back(server);
				return;
			}
			continue;
		}

		if (line.find("location") == 0 && line.find("{") != std::string::npos) {
			size_t start = line.find_first_not_of(" \t", 8);
			size_t end = line.find("{");
			std::string path = trim(line.substr(start, end - start));

			LocationConfig loc(path);
			server.addLocation(loc);
			currentLocation = const_cast<LocationConfig*>(&server.getLocations().back());
			inLocation = true;
			continue;
		}

		size_t pos = line.find_first_of(" \t");
		if (pos == std::string::npos)
			continue;

		std::string key = trim(line.substr(0, pos));
		std::string value = trim(line.substr(pos + 1));

		if (value[value.length() - 1] == ';')
			value = value.substr(0, value.length() - 1);

		if (inLocation && currentLocation) {
			if (key == "root")
				currentLocation->setRoot(value);
			else if (key == "index")
				currentLocation->setIndex(value);
			else if (key == "autoindex")
				currentLocation->setAutoindex(value == "on");
			else if (key == "allow_methods") {
				std::vector<std::string> methods = Utils::split(value, ' ');
				for (size_t i = 0; i < methods.size(); i++)
					currentLocation->addAllowedMethod(methods[i]);
			}
			else if (key == "upload_path")
				currentLocation->setUploadPath(value);
			else if (key == "cgi_extension")
				currentLocation->setCgiExtension(value);
			else if (key == "cgi_path")
				currentLocation->setCgiPath(value);
			else if (key == "return")
				currentLocation->setRedirect(value);
		} else {
			if (key == "listen")
				server.setPort(Utils::stringToInt(value));
			else if (key == "host")
				server.setHost(value);
			else if (key == "server_name") {
				std::vector<std::string> names = Utils::split(value, ' ');
				for (size_t i = 0; i < names.size(); i++)
					server.addServerName(names[i]);
			}
			else if (key == "error_page") {
				std::vector<std::string> parts = Utils::split(value, ' ');
				if (parts.size() >= 2) {
					int code = Utils::stringToInt(parts[0]);
					server.setErrorPage(code, parts[1]);
				}
			}
			else if (key == "client_max_body_size")
				server.setClientMaxBodySize(Utils::stringToSize(value));
		}
	}
}
