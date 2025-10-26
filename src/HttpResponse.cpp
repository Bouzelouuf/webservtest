#include "HttpResponse.hpp"
#include "Utils.hpp"
#include <sstream>

HttpResponse::HttpResponse() : statusCode(200) {
	statusMessage = HttpResponse::getStatusMessage(200);
}

HttpResponse::~HttpResponse() {}

void HttpResponse::setStatusCode(int code) {
	statusCode = code;
	statusMessage = HttpResponse::getStatusMessage(code);
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
	headers[name] = value;
}

void HttpResponse::setBody(const std::string& content) {
	body = content;
	setHeader("Content-Length", Utils::intToString(content.length()));
}

void HttpResponse::setBody(const std::string& content, const std::string& contentType) {
	body = content;
	setHeader("Content-Type", contentType);
	setHeader("Content-Length", Utils::intToString(content.length()));
}

void HttpResponse::buildResponse() {
	std::stringstream ss;
	ss << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";

	for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it) {
		ss << it->first << ": " << it->second << "\r\n";
	}

	ss << "\r\n";
	ss << body;

	rawResponse = ss.str();
}

const std::string& HttpResponse::getRawResponse() {
	if (rawResponse.empty())
		buildResponse();
	return rawResponse;
}

void HttpResponse::clear() {
	statusCode = 200;
	statusMessage = HttpResponse::getStatusMessage(200);
	headers.clear();
	body.clear();
	rawResponse.clear();
}

std::string HttpResponse::getStatusMessage(int code) {
	switch (code) {
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		default: return "Unknown";
	}
}

std::string HttpResponse::getErrorPage(int code, const std::string& customPath) {
	if (!customPath.empty() && Utils::fileExists(customPath))
		return Utils::readFile(customPath);

	std::stringstream ss;
	ss << "<!DOCTYPE html>\n<html>\n<head>\n<title>" << code << " " << getStatusMessage(code) << "</title>\n";
	ss << "<style>body{font-family:Arial,sans-serif;text-align:center;padding:50px;}";
	ss << "h1{font-size:50px;margin:0;}p{font-size:20px;color:#666;}</style>\n";
	ss << "</head>\n<body>\n";
	ss << "<h1>" << code << "</h1>\n";
	ss << "<p>" << getStatusMessage(code) << "</p>\n";
	ss << "<hr><p>WebServ/1.0</p>\n";
	ss << "</body>\n</html>";

	return ss.str();
}
