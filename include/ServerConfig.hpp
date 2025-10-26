#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include "LocationConfig.hpp"

class ServerConfig {
private:
	int port;
	std::string host;
	std::vector<std::string> serverNames;
	std::map<int, std::string> errorPages;
	size_t clientMaxBodySize;
	std::vector<LocationConfig> locations;

public:
	ServerConfig();
	~ServerConfig();

	void setPort(int p);
	void setHost(const std::string& h);
	void addServerName(const std::string& name);
	void setErrorPage(int code, const std::string& path);
	void setClientMaxBodySize(size_t size);
	void addLocation(const LocationConfig& loc);

	int getPort() const;
	const std::string& getHost() const;
	const std::vector<std::string>& getServerNames() const;
	const std::map<int, std::string>& getErrorPages() const;
	size_t getClientMaxBodySize() const;
	const std::vector<LocationConfig>& getLocations() const;
	const LocationConfig* matchLocation(const std::string& path) const;
};

#endif
