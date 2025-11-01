#ifndef FILEHANDLER_HPP
#define FILEHANDLER_HPP

#include "Request.hpp"
#include "Response.hpp"
#include <string>

class FileHandler
{
    public:
        static Response handle(const HttpRequest& res, const std::string& root_dir);
    private:
        static std::string getContentType(const std::string &path);
        static std::string readFile(const std::string &path);
        static bool fileExists(const std::string &path);
        static bool canRead(const std::string& path);
};

#endif