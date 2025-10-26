#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HttpResponse {
private:
	int statusCode;
	std::string statusMessage;
	std::map<std::string, std::string> headers;
	std::string body;
	std::string rawResponse;

	void buildResponse();

public:
	HttpResponse();
	~HttpResponse();

	void setStatusCode(int code);
	void setHeader(const std::string& name, const std::string& value);
	void setBody(const std::string& content);
	void setBody(const std::string& content, const std::string& contentType);

	const std::string& getRawResponse();
	void clear();

	static std::string getErrorPage(int code, const std::string& customPath = "");
	static std::string getStatusMessage(int code);
};

#endif
