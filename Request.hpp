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

struct UploadedFile
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
    UploadedFile() : size(0){}

    bool saveTo(const std::string& directory) const
    {
        if (filename.empty() || data.empty())
            return false;
        std::string filepath = directory;
        if (!filepath.empty() && filepath[filepath.length() - 1] != '/')
            filepath += "/";
        filepath += filename;
        //construit juste le chemin complet du file
        //dossier : uploads - file : test.txt = uploads/test.txt

        std::ofstream file(filepath.c_str(), std::ios::binary);
        //ouvrir le file en monde binaire
        //std::ios::binary est crucial pour file binary
        if (!file.is_open())
            return false;
        file.write(data.c_str(), data.size());
        //on utilise write pour du binaire pas << 
        file.close();
        return true;
    }
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
        static const size_t MAX_BODY_SIZE = 10485760;
        //10 mb

        std::string path;
        std::map<std::string, std::string> query_params;

        std::map<std::string, std::string> post_params;
        std::map<std::string, std::string> cookies;
    
        std::map<std::string, UploadedFile> uploaded_files;
        bool is_chunked;


    public:
        HttpRequest();
        void parse(std::string request);
        std::string getMethod()const;
        std::string getUri()const;
        std::string getVersion()const;
        std::string getBody()const;
        void displayHeaders()const;
        std::string getHeader(std::string param)const;
        size_t getContentLength() const;
        bool hasContentLength()const;
        void validateMethod();
        bool isMethodValid(const std::string& method)const;
        const std::map<std::string, std::string>& getQueryParams() const;
        std::string getPath()const;
        std::string getQueryParam(const std::string& key)const;
        std::string getPostParam(const std::string& key) const;

        const std::map<std::string, std::string>& getPostParams() const;
        std::string getCookie(const std::string &key)const;
        const std::map<std::string, std::string>& getCookies() const;
    
        bool hasFile(const std::string& field_name) const;
        UploadedFile getFile(const std::string& field_name) const;
        std::vector<std::string> getFileNames() const;

        bool isRegularFile(const std:: string &path)const;
        bool isDirectory(const std::string& path_directory) const;
        bool fileExists(const std::string& path) const;
        size_t getFileSize(const std::string& path) const;
        bool canRead(const std::string& path) const;
        bool canWrite(const std::string& path) const;
        bool canExecute(const std::string& path) const;

        bool isChunked()const;

        void parsedChunkedBody(const std::string& raw_body);
        std::string dechunkBody(const std::string& chunked_data);
//pas sur que ce doit etre la , peut etre private
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