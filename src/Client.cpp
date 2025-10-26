#include "Client.hpp"
#include "Utils.hpp"
#include "CGI.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

Client::Client(int fd, int servFd)
	: socketFd(fd), serverFd(servFd), writtenBytes(0),
	  requestComplete(false), responseReady(false), lastActivity(std::time(NULL)) {}

Client::~Client() {
	close();
}

int Client::getSocketFd() const {
	return socketFd;
}

int Client::getServerFd() const {
	return serverFd;
}

time_t Client::getLastActivity() const {
	return lastActivity;
}

void Client::updateActivity() {
	lastActivity = std::time(NULL);
}

bool Client::readRequest() {
    char buffer[4096];
    ssize_t bytes = recv(socketFd, buffer, sizeof(buffer), 0);

    std::cout << "readRequest() called, bytes received: " << bytes << std::endl;  // ✅ AJOUTE

    if (bytes <= 0)
        return false;

    updateActivity();
    readBuffer.append(buffer, bytes);

    std::cout << "readBuffer size: " << readBuffer.size() << std::endl;  // ✅ AJOUTE

    request.parse(readBuffer);
    
    std::cout << "isRequestComplete: " << request.isComplete() << std::endl;  // ✅ AJOUTE
    
    if (request.isComplete()) {
        requestComplete = true;
        std::cout << "REQUEST COMPLETE!" << std::endl;  // ✅ AJOUTE
        return true;
    }

    return true;
}

bool Client::writeResponse() {
	if (writeBuffer.empty())
		return false;

	ssize_t bytes = send(socketFd, writeBuffer.c_str() + writtenBytes,
	                     writeBuffer.length() - writtenBytes, 0);

	if (bytes <= 0)
		return false;

	updateActivity();
	writtenBytes += bytes;

	return true;
}

void Client::processRequest(const ServerConfig& config) {
	if (!requestComplete || responseReady)
		return;

	const std::string& method = request.getMethod();
	std::string uri = request.getUri();

	size_t queryPos = uri.find('?');
	std::string queryString;
	if (queryPos != std::string::npos) {
		queryString = uri.substr(queryPos + 1);
		uri = uri.substr(0, queryPos);
	}

	uri = Utils::urlDecode(uri);

	const LocationConfig* location = config.matchLocation(uri);

	if (!location) {
		response.setStatusCode(404);
		response.setBody(HttpResponse::getErrorPage(404), "text/html");
		writeBuffer = response.getRawResponse();
		responseReady = true;
		return;
	}

	if (!location->getRedirect().empty()) {
		response.setStatusCode(301);
		response.setHeader("Location", location->getRedirect());
		response.setBody("");
		writeBuffer = response.getRawResponse();
		responseReady = true;
		return;
	}

	if (!location->isMethodAllowed(method)) {
		response.setStatusCode(405);
		response.setBody(HttpResponse::getErrorPage(405), "text/html");
		writeBuffer = response.getRawResponse();
		responseReady = true;
		return;
	}

	if (request.getBody().length() > config.getClientMaxBodySize()) {
		response.setStatusCode(413);
		response.setBody(HttpResponse::getErrorPage(413), "text/html");
		writeBuffer = response.getRawResponse();
		responseReady = true;
		return;
	}

	std::string root = location->getRoot();
	if (root.empty())
		root = ".";

	std::string fullPath = root + uri;

	if (method == "GET") {
		if (!location->getCgiExtension().empty() &&
		    fullPath.find(location->getCgiExtension()) != std::string::npos) {
			CGI cgi(fullPath, location->getCgiPath(), request);
			std::string output = cgi.execute(queryString);

			if (output.empty()) {
				response.setStatusCode(500);
				response.setBody(HttpResponse::getErrorPage(500), "text/html");
			} else {
				size_t headerEnd = output.find("\r\n\r\n");
				if (headerEnd != std::string::npos) {
					std::string headers = output.substr(0, headerEnd);
					std::string body = output.substr(headerEnd + 4);

					std::istringstream iss(headers);
					std::string line;
					while (std::getline(iss, line)) {
						if (!line.empty() && line[line.length() - 1] == '\r')
							line = line.substr(0, line.length() - 1);
						size_t colon = line.find(':');
						if (colon != std::string::npos) {
							std::string key = Utils::trim(line.substr(0, colon));
							std::string value = Utils::trim(line.substr(colon + 1));
							response.setHeader(key, value);
						}
					}
					response.setStatusCode(200);
					response.setBody(body);
				} else {
					response.setStatusCode(200);
					response.setBody(output, "text/html");
				}
			}
		} else if (Utils::isDirectory(fullPath)) {
			if (fullPath[fullPath.length() - 1] != '/')
				fullPath += "/";

			std::string indexFile = fullPath + location->getIndex();

			if (!location->getIndex().empty() && Utils::fileExists(indexFile)) {
				std::string content = Utils::readFile(indexFile);
				response.setStatusCode(200);
				response.setBody(content, Utils::getMimeType(indexFile));
			} else if (location->getAutoindex()) {
				std::string listing = Utils::getDirectoryListing(fullPath, uri);
				response.setStatusCode(200);
				response.setBody(listing, "text/html");
			} else {
				response.setStatusCode(403);
				response.setBody(HttpResponse::getErrorPage(403), "text/html");
			}
		} else if (Utils::fileExists(fullPath)) {
			std::string content = Utils::readFile(fullPath);
			if (content.empty()) {
				response.setStatusCode(500);
				response.setBody(HttpResponse::getErrorPage(500), "text/html");
			} else {
				response.setStatusCode(200);
				response.setBody(content, Utils::getMimeType(fullPath));
			}
		} else {
			std::map<int, std::string>::const_iterator it = config.getErrorPages().find(404);
			std::string errorPage = (it != config.getErrorPages().end()) ?
			                        HttpResponse::getErrorPage(404, it->second) :
			                        HttpResponse::getErrorPage(404);
			response.setStatusCode(404);
			response.setBody(errorPage, "text/html");
		}
	}
	else if (method == "POST")
	{
		std::string uploadPath = location->getUploadPath();
		if (uploadPath.empty())
			uploadPath = root;

		std::string contentType = request.getHeader("Content-Type");
		if (contentType.find("multipart/form-data") != std::string::npos) {
			std::vector<std::string> fileNames = request.getFileNames();
			if (!fileNames.empty()) {
				for (size_t i = 0; i < fileNames.size(); ++i) {
					UploadedFile file = request.getFile(fileNames[i]);
					
					if (file.saveTo(uploadPath)) {
						std::cout << "File saved: " << file.filename << std::endl;
					}
				}
				response.setStatusCode(201);
				response.setBody("Files uploaded successfully", "text/plain");
			} else {
				response.setStatusCode(400);
				response.setBody("No files in request", "text/plain");
			}
		}
		else
		{
			std::string savePath = uploadPath + "/upload.txt";
			if (Utils::writeFile(savePath, request.getBody())) {
				response.setStatusCode(201);
				response.setBody("Data saved successfully", "text/plain");
			} else {
				response.setStatusCode(500);
				response.setBody(HttpResponse::getErrorPage(500), "text/html");
			}
		}
	}
	else if (method == "DELETE") {
		if (Utils::fileExists(fullPath) && !Utils::isDirectory(fullPath)) {
			if (Utils::deleteFile(fullPath)) {
				response.setStatusCode(204);
				response.setBody("");
			} else {
				response.setStatusCode(500);
				response.setBody(HttpResponse::getErrorPage(500), "text/html");
			}
		} else {
			response.setStatusCode(404);
			response.setBody(HttpResponse::getErrorPage(404), "text/html");
		}
	} else {
		response.setStatusCode(501);
		response.setBody(HttpResponse::getErrorPage(501), "text/html");
	}

	writeBuffer = response.getRawResponse();
	responseReady = true;
}

bool Client::isRequestComplete() const {
	return requestComplete;
}

bool Client::isResponseReady() const {
	return responseReady;
}

bool Client::isWriteComplete() const {
	return writtenBytes >= writeBuffer.length();
}

void Client::close() {
	if (socketFd >= 0) {
		::close(socketFd);
		socketFd = -1;
	}
}
