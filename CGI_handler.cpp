#include "CGI_handler.hpp"

// crea le variabili d'ambiente
std::vector<char *> CGIHandler::createEnv(const HttpRequest &request, const std::string &script_path, const std::string &root)
{
    std::vector<std::string> env;
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("REQUEST_METHOD=" + request.getMethod());
    env.push_back("QUERY_STRING=" + request.getQuery());
    std::ostringstream oss;
    oss << request.getContentLength();
    env.push_back("CONTENT_LENGTH=" + oss.str());
    env.push_back("CONTENT_TYPE=" + request.getHeader("Content-Type"));
    env.push_back("SCRIPT_FILENAME=" + root + script_path);
    env.push_back("PATH_INFO=" + script_path);
    env.push_back("SERVER_PROTOCOL=" + request.getVersion());
    env.push_back("SERVER_SOFTWARE=webserv");
    if (!request.getHeader("Cookie").empty())
        env.push_back("HTTP_COOKIE=" + request.getHeader("Cookie"));

    std::vector<char *> env_c;
    for (size_t i = 0; i < env.size(); i++)
        env_c.push_back(strdup(env[i].c_str()));
    env_c.push_back(NULL);
    return env_c;
}

void CGIHandler::freeEnv(std::vector<char *> &env)
{
    for (size_t i = 0; i < env.size(); i++)
        free(env[i]);
    env.clear();
}

// spawn della CGI in modalità non bloccante
CgiProcess CGIHandler::spawnCgi(const HttpRequest &request, const std::string &script_path, const std::string &root)
{
    int pipe_in[2], pipe_out[2];
    if (pipe(pipe_in) < 0 || pipe(pipe_out) < 0)
        throw std::runtime_error("pipe failed");

    pid_t pid = fork();
    if (pid < 0)
        throw std::runtime_error("fork failed");

    if (pid == 0) // figlio CGI
    {
        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);
        close(pipe_in[1]);
        close(pipe_out[0]);

        std::vector<char *> env = createEnv(request, script_path, root);
        char *args[2];
        args[0] = (char *)script_path.c_str();
        args[1] = NULL;

        execve(script_path.c_str(), args, env.data());
        perror("execve failed");
        freeEnv(env);
        exit(1);
    }

    // padre
    close(pipe_in[0]);
    close(pipe_out[1]);
    fcntl(pipe_in[1], F_SETFL, O_NONBLOCK);
    fcntl(pipe_out[0], F_SETFL, O_NONBLOCK);

    CgiProcess cgi;
    cgi.pid = pid;
    cgi.pipe_in = pipe_in[1];
    cgi.pipe_out = pipe_out[0];
    cgi.stdin_closed = false;
    if (request.getMethod() == "POST")
        cgi.write_buffer = request.getBody();

    return cgi;
}

// chiamata da poll() per scrivere verso CGI
void CGIHandler::handleWrite(CgiProcess &cgi)
{
    if (!cgi.stdin_closed && !cgi.write_buffer.empty())
    {
        ssize_t written = write(cgi.pipe_in, cgi.write_buffer.c_str(), cgi.write_buffer.size());
        if (written > 0)
            cgi.write_buffer.erase(0, written);
        if (cgi.write_buffer.empty())
        {
            close(cgi.pipe_in);
            cgi.stdin_closed = true;
        }
    }
}

// chiamata da poll() per leggere output CGI
void CGIHandler::handleRead(CgiProcess &cgi)
{
    char buf[1024];
    ssize_t n = read(cgi.pipe_out, buf, sizeof(buf));
    if (n > 0)
        cgi.read_buffer.append(buf, n);
    else if (n == 0)
        close(cgi.pipe_out); // fine output
}

// controllo se il processo CGI è terminato
bool CGIHandler::isFinished(CgiProcess &cgi)
{
    int status;
    pid_t result = waitpid(cgi.pid, &status, WNOHANG);
    return result > 0;
}
