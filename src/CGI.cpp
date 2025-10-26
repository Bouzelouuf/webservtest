#include "CGI.hpp"
#include "Utils.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdlib>

CGI::CGI(const std::string& script, const std::string& cgi, HttpRequest& req)
	: scriptPath(script), cgiPath(cgi), request(req) {}

CGI::~CGI() {}

void CGI::setupEnvironment(const std::string& queryString) {
	env["REQUEST_METHOD"] = request.getMethod();
	env["QUERY_STRING"] = queryString;
	env["SCRIPT_FILENAME"] = scriptPath;
	env["REDIRECT_STATUS"] = "200";

	std::string contentLength = request.getHeader("Content-Length");
	if (!contentLength.empty())
		env["CONTENT_LENGTH"] = contentLength;

	std::string contentType = request.getHeader("Content-Type");
	if (!contentType.empty())
		env["CONTENT_TYPE"] = contentType;

	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
}

char** CGI::getEnvArray() {
	char** envp = new char*[env.size() + 1];
	int i = 0;

	for (std::map<std::string, std::string>::iterator it = env.begin(); it != env.end(); ++it) {
		std::string envVar = it->first + "=" + it->second;
		envp[i] = new char[envVar.length() + 1];
		std::strcpy(envp[i], envVar.c_str());
		i++;
	}
	envp[i] = NULL;

	return envp;
}

void CGI::freeEnvArray(char** envp) {
	for (int i = 0; envp[i] != NULL; i++)
		delete[] envp[i];
	delete[] envp;
}

std::string CGI::execute(const std::string& queryString) {
	setupEnvironment(queryString);

	int pipefd[2];
	int inputfd[2];

	if (pipe(pipefd) == -1 || pipe(inputfd) == -1)
		return "";

	pid_t pid = fork();
	if (pid == -1) {
		close(pipefd[0]);
		close(pipefd[1]);
		close(inputfd[0]);
		close(inputfd[1]);
		return "";
	}

	if (pid == 0) {
		close(pipefd[0]);
		close(inputfd[1]);

		dup2(inputfd[0], STDIN_FILENO);
		dup2(pipefd[1], STDOUT_FILENO);

		close(inputfd[0]);
		close(pipefd[1]);

		char** envp = getEnvArray();
		char* argv[] = { const_cast<char*>(cgiPath.c_str()),
		                 const_cast<char*>(scriptPath.c_str()),
		                 NULL };

		execve(cgiPath.c_str(), argv, envp);
		exit(1);
	}

	close(pipefd[1]);
	close(inputfd[0]);

	const std::string& body = request.getBody();
	if (!body.empty())
		write(inputfd[1], body.c_str(), body.length());
	close(inputfd[1]);

	std::string output;
	char buffer[4096];
	ssize_t bytes;

	while ((bytes = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
		buffer[bytes] = '\0';
		output += buffer;
	}

	close(pipefd[0]);

	int status;
	waitpid(pid, &status, 0);

	return output;
}
