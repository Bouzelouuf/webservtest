#ifndef UPLOADEDFILE_HPP
#define UPLOADEDFILE_HPP

#include "Request.hpp"
#include "Response.hpp"
#include <string>

class UploadHandler
{
    public:
        static Response handle(const HttpRequest& req, const std::string &upload_dir);
    private:
        static bool ensureDirectoryExists(const std::string& dir);
        //static bool saveFile(const std::string& filepath, const std::string& content);
        //static std::string intToString(int n);
        static bool saveFile(const ParsedFile& file, const std::string& dir);

};

#endif 