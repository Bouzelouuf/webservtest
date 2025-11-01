#include "FileHandler.hpp"

 Response FileHandler::handle(const HttpRequest& res, const std::string& root_dir)
{
    Response resp(res);
    std::string path = root_dir + res.getPath(); // construction du path complet

    if (path[path.length() - 1] == '/')
        path += "index.html";// ajout de index html apres / si trouver
    if (!fileExists(path))
    {
        resp.setStatus(404);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>404 Not Found</h1></body></html>");
        return resp;
    }
    if (!canRead(path))
    {
        resp.setStatus(403);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>Forbidden</h1></body></html>");
        return resp;
    }
    std::string content = readFile(path);
    //lire file
    
    //reponse positive
    resp.setStatus(200);
    resp.addHeader("Content-Type", getContentType(path));
    resp.setBody(content);
    return resp;
}


 std::string FileHandler::getContentType(const std::string &path)
{
    if (path.find(".html") != std::string::npos)
        return "text/html";
    if (path.find(".css") != std::string::npos)
        return ("text/css");
    if (path.find(".js") != std::string::npos)
        return ("application/javascript");
    if (path.find(".png") != std::string::npos)
        return ("image/png");
    if (path.find(".jpg") != std::string::npos)
        return ("image/jpeg");
    if (path.find(".gif") != std::string::npos)
        return ("image/gif");
    return "application/octet-stream";
}

 std::string FileHandler::readFile(const std::string &path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
        return "";
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

 bool FileHandler::fileExists(const std::string &path)
{
    return (access(path.c_str(), F_OK) == 0);
}

 bool FileHandler::canRead(const std::string& path)
{
    return (access(path.c_str(), R_OK) == 0);
}