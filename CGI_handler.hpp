#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <iostream>
#include <unistd.h>
#include <string.h>
#include <map>
#include <vector>
#include <sys/wait.h>
#include "Request.hpp"
#include "Response.hpp"
#include <fcntl.h>

struct CgiProcess
{
    pid_t pid;
    int pipe_in;              // scrittura verso CGI
    int pipe_out;             // lettura da CGI
    std::string write_buffer; // dati POST da inviare
    std::string read_buffer;  // output CGI accumulato
    bool stdin_closed;
};

class CGIHandler
{
public:
    CGIHandler();
    ~CGIHandler();
    CGIHandler(const CGIHandler &other);
    CGIHandler &operator=(const CGIHandler &other);
    static std::string execute(const HttpRequest &request, const std::string &script_path, const std::string &root);
    static CgiProcess spawnCgi(const HttpRequest &request, const std::string &script_path, const std::string &root);
    static void handleWrite(CgiProcess &cgi);
    static void handleRead(CgiProcess &cgi);
    static bool isFinished(CgiProcess &cgi);

private:
    static std::vector<char *> createEnv(const HttpRequest &request, const std::string &script_path, const std::string &root);
    static void freeEnv(std::vector<char *> &env);
};

#endif