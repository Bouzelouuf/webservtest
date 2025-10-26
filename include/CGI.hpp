#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <map>
#include "HttpRequest.hpp"

class CGI {
private:
	std::string scriptPath;
	std::string cgiPath;
	HttpRequest& request;
	std::map<std::string, std::string> env;

	void setupEnvironment(const std::string& queryString);
	char** getEnvArray();
	void freeEnvArray(char** envp);

public:
	CGI(const std::string& script, const std::string& cgi, HttpRequest& req);
	~CGI();

	std::string execute(const std::string& queryString);
};

#endif
