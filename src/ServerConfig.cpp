#include "ServerConfig.hpp"

ServerConfig::ServerConfig() : port(8080), host("0.0.0.0"), clientMaxBodySize(1048576) {}

ServerConfig::~ServerConfig() {}

void ServerConfig::setPort(int p) {
	port = p;
}

void ServerConfig::setHost(const std::string& h) {
	host = h;
}

void ServerConfig::addServerName(const std::string& name) {
	serverNames.push_back(name);
}

void ServerConfig::setErrorPage(int code, const std::string& path) {
	errorPages[code] = path;
}

void ServerConfig::setClientMaxBodySize(size_t size) {
	clientMaxBodySize = size;
}

void ServerConfig::addLocation(const LocationConfig& loc) {
	locations.push_back(loc);
}

int ServerConfig::getPort() const {
	return port;
}

const std::string& ServerConfig::getHost() const {
	return host;
}

const std::vector<std::string>& ServerConfig::getServerNames() const {
	return serverNames;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const {
	return errorPages;
}

size_t ServerConfig::getClientMaxBodySize() const {
	return clientMaxBodySize;
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const {
	return locations;
}

const LocationConfig* ServerConfig::matchLocation(const std::string& path) const {
	const LocationConfig* bestMatch = NULL;
	size_t bestMatchLen = 0;

	for (size_t i = 0; i < locations.size(); i++) {
		const std::string& locPath = locations[i].getPath();

		if (path.find(locPath) == 0) {
			if (locPath.length() > bestMatchLen) {
				bestMatch = &locations[i];
				bestMatchLen = locPath.length();
			}
		}
	}

	return bestMatch;
}
