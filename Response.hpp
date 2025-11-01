#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <unistd.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include "Request.hpp"

class Response
{
    private:
        int status_code;
        std::string status_message;
        std::map<std::string, std::string> headers;
        std::string body;
        const HttpRequest *request;
        std::string http_version;
    public:
        Response(const HttpRequest& req);
        Response();
        ~Response();
        std::string generate();
        void setStatus(int code);
        void setBody(const std::string& content);
        void addHeader(const std::string &key, const std::string &value);
};
#endif