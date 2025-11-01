#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

struct ParsedFile
{
    std::string field_name;
    //pour le champ html <input name="avatar">
    std::string filename;
    //nom du file original sur user pc
    std::string content_type;
    //le type, image, jpegapp, pdf text etc....
    std::string data;
    //les donnees binaire du file stocker dans le string
    //std::string een c++ peut conteni nimporte quel octets
    size_t size;
    //bool saveTo(const std::string& directory)const;
    //method pour save le file
    ParsedFile() : size(0){}
};

class HttpRequest
{
    private:
        std::string method;
        std::string uri;
        std::string version;
        std::map<std::string, std::string> headers;
        std::string body;
        size_t content_length;
        bool has_content_length;
        std::string query;
        static const size_t MAX_BODY_SIZE = 10485760;
        //10 mb

        std::string path;
        std::map<std::string, std::string> query_params;

        std::map<std::string, std::string> post_params;
        std::map<std::string, std::string> cookies;
    
        std::map<std::string, ParsedFile> parsed_files;
        bool is_chunked;
        bool is_cgi;
        int error_code;
        std::string error_message;
    public:
        HttpRequest();
        void parse(std::string request);
        std::string getMethod()const;
        std::string getUri()const;
        std::string getVersion()const;
        std::string getBody()const;
        std::string getHeader(std::string param)const;
        size_t getContentLength() const;
        std::string getPath()const;
        std::string getQuery() const;
        std::string getQueryParam(const std::string& key)const;
        std::string getPostParam(const std::string& key) const;
        const std::map<std::string, std::string>& getPostParams() const;
        std::string getCookie(const std::string &key)const;
        const std::map<std::string, std::string>& getCookies() const;
        const std::map<std::string, std::string>& getQueryParams() const;
        bool hasFile(const std::string& field_name) const;
        ParsedFile getFile(const std::string& field_name) const;
        std::vector<std::string> getFileNames() const;
        size_t getFileSize(const std::string& path) const;
        void displayHeaders()const;
        bool hasContentLength()const;
        void validateMethod();
        bool isMethodValid(const std::string& method)const;
        

        bool isRegularFile(const std:: string &path)const;
        bool isDirectory(const std::string& path_directory) const;
        bool fileExists(const std::string& path) const;
        
        bool canRead(const std::string& path) const;
        bool canWrite(const std::string& path) const;
        bool canExecute(const std::string& path) const;

        bool isChunked()const;
        bool isCgi(const std::string&path);
        void parsedChunkedBody(const std::string& raw_body);
        std::string dechunkBody(const std::string& chunked_data);
//pas sur que ce doit etre la , peut etre private
        size_t getFileCount(const std::string& field_name) const;

        bool isValid()const;
        int getErrorCode()const;
        std::string getErrorMessage()const;
    private:
        void validateContentLength();
        std::string toLower(const std::string& str)const;
        void validateVersion();
        void validateUri();
        void validateHost();
        void parseQueryString();
        std::string urlDecode(const std::string& str)const;
        void parsePostBody();
        void parseCookies();
        std::string extractBoundary(const std::string& content_type) const;
        void parseMultipartBody(const std::string& boundary);
        std::string extractValueFromHeader(const std::string& header, const std::string& key) const;
        void parseSinglePart(const std::string& part);

};
#endif