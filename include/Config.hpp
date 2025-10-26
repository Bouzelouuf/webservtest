#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include "ServerConfig.hpp"

class Config {
private:
	std::vector<ServerConfig> servers;
	std::string configFile;

	void parseConfigFile(const std::string& filename);
	void parseServer(std::ifstream& file, std::string& line);
	std::string trim(const std::string& str);

public:
	Config();
	Config(const std::string& filename);
	~Config();

	const std::vector<ServerConfig>& getServers() const;
	void loadConfig(const std::string& filename);
};

#endif
