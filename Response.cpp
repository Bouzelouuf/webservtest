#include "Response.hpp"

Response::Response(const HttpRequest& req): status_code(200), status_message("OK"), request(&req), http_version("HTTP/1.1")
{
    addHeader("Server", "Webserv/1.0");
    addHeader("Connection", "close");
}

Response::Response():status_code(400),status_message("Bad Request"),request(NULL),http_version("HTTP/1.1")
{
    addHeader("Server", "Webserv/1.0");
    addHeader("Connection", "close");
}

Response::~Response(){}

void Response::setStatus(int code)
{
    status_code = code;
    switch(code)
    {
        case 200:
            status_message = "OK";
            break;
        // 200 == success (GET reussi ,file trovuer et envoyer)
        case 201:
            status_message = "Created";
            break;
        // quand une ressource est creee avec success (post qui cree/ upload reussi)
        case 204:
            status_message = "No Content";
            break;
        // succes mais pas de contenu a renvouer 
        case 302:
        //redirection temp , nav va auto sur new url 
            status_message = "Found";
            break;
        case 400:
        //requete mal formee , utiliser dans un throw de Request uri invalid, headers malformee
            status_message = "Bad Request";
            break;
        case 403:
        //file existe mais pas la perm dy access au directory , ou file sans perm daccess
            status_message = "Forbidden";
            break;
        case 404:
        // file ou ressource nexiste pas ( get / page_inexistant.html)
            status_message = "Not Found";
            break;
        case 405:
        // method pas autoriser (config dit : cette route accepte seulement GET mais client envoit pos ??!)
            status_message = "Method Not Allowed";
            break;
        case 413:
        //body trop grand ( dans content length > max body size)
            status_message = "Payload Too Large";
            break;

        case 500:
        // error coter server ( except non gerer , bug code, crash traitement..)
            status_message = "Internal Server Error";
            break;
        case 505:
        // version http non supporter, client envois http 2.0 / 0.9 etc
            status_message = "HTTP Version Not Supported";
            break;
        default:
            status_message = "Unknown";
            break;

    }
}

void Response::addHeader(const std::string &key, const std::string &value)
{
    headers[key] = value;
}

void Response::setBody(const std::string &content)
{
    body = content;
    std::ostringstream osscontent_len;
    osscontent_len << content.length();
    std::string content_len = osscontent_len.str();
    addHeader("Content-Length", content_len);
}

std::string Response::generate()
{
    std::ostringstream response;
    std::string version;
    if (request != NULL)
        version = request->getVersion();
    else
        version = http_version;
    response << version  << " " << status_code << " " << status_message << "\r\n";
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it)
        response << it->first << ": " << it->second << "\r\n";
    response << "\r\n";
    response << body;
    return response.str();
}