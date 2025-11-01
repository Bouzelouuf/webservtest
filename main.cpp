#include "Webserv.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "FileHandler.hpp"
//#include "UploadHandler.hpp"

std::string intToString(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

int main()
{
    std::cout << "Initializing Webserv..." << std::endl;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        std::cerr << ("socket error");
        return 1;
    }
    struct pollfd listen_poll = {listen_fd, POLLIN, 0};

    int option = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
    {
        std::cerr << ("setsockopt error");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        std::cerr << ("bind error");
        return 1;
    }
    if (listen(listen_fd, SOMAXCONN) < 0)
    {
        std::cerr << ("listen error");
        return 1;
    }
    int flags = fcntl(listen_fd, F_GETFL, 0);
    fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK);
    std::cout << "Webserv is running/ready" << std::endl;
    std::vector<struct pollfd> fds;
    std::map<int, int> cgi_pipes;
    std::map<int, std::string> send_buffers;
    std::map<int, std::string> recv_buffers;
    fds.push_back(listen_poll);

    while (1)
    {
        int monitored_fds = poll(fds.data(), fds.size(), -1);
        if (monitored_fds < 0)
        {
            std::cerr << "poll error" << std::endl;
            break;
        }
        for (size_t i = 0; i < fds.size(); i++)
        {
            int fd = fds[i].fd;
            if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                std::cerr << "Error/hangup on fd " << fd << ", closing\n";
                close(fd);
                cgi_pipes.erase(fd);
                send_buffers.erase(fd);
                recv_buffers.erase(fd);
                fds.erase(fds.begin() + i);
                --i;
                continue;
            }

            if (fd == listen_fd && (fds[i].revents & POLLIN))
            {
                int client_fd = accept(listen_fd, NULL, NULL);
                if (client_fd < 0)
                {
                    std::cerr << "accept error" << std::endl;
                    continue;
                }
                fcntl(client_fd, F_SETFL, O_NONBLOCK);
                struct pollfd client_poll = {client_fd, POLLIN, 0};
                fds.push_back(client_poll);
                continue;
            }
            else if (fds[i].revents & POLLIN && cgi_pipes.find(fd) != cgi_pipes.end())
            {
                char buf[1024];
                int bytes_read = read(fd, buf, sizeof(buf) - 1);
                if (bytes_read > 0)
                {
                    buf[bytes_read] = '\0';
                    int client_fd = cgi_pipes[fd];
                    send_buffers[client_fd] += std::string(buf, bytes_read);
                    for (std::vector<struct pollfd>::iterator it = fds.begin(); it != fds.end(); ++it)
                    {
                        if (it->fd == client_fd)
                            it->events |= POLLOUT;
                    }
                }
                else
                {
                    close(fd);
                    int client_fd = cgi_pipes[fd];
                    close(client_fd);
                    cgi_pipes.erase(fd);
                    send_buffers.erase(client_fd);
                    recv_buffers.erase(client_fd);
                    fds.erase(fds.begin() + i);
                    --i;
                    continue;
                }
            }
            else if (fds[i].revents & POLLIN && (fd != listen_fd))
            {
                char buf[1024];
                int bytes_received = recv(fd, buf, sizeof(buf) - 1, 0);
                if (bytes_received <= 0)
                {
                    close(fd);
                    send_buffers.erase(fd);
                    recv_buffers.erase(fd);
                    fds.erase(fds.begin() + i);
                    --i;
                    continue;
                }
                buf[bytes_received] = '\0';
                recv_buffers[fd] += std::string(buf, bytes_received);
                size_t header_end = recv_buffers[fd].find("\r\n\r\n");
                if (header_end == std::string::npos)
                    continue;
                std::string headers_part = recv_buffers[fd].substr(0, header_end + 4);
                size_t content_length_pos = headers_part.find("Content-Length:");
                size_t transfer_encoding_pos = headers_part.find("Transfer-Encoding:");
                size_t content_length = 0;
                bool has_content_length = false;
                bool is_chunked = false;
                if (transfer_encoding_pos != std::string::npos)
                {
                    size_t line_end = headers_part.find("\r\n", transfer_encoding_pos);
                    std::string value = headers_part.substr(transfer_encoding_pos, line_end - transfer_encoding_pos);
                    if (value.find("chunked") != std::string::npos)
                        is_chunked = true;
                }
                if (content_length_pos != std::string::npos)
                {
                    content_length_pos += 15;
                    while (content_length_pos < headers_part.size() &&
                        (headers_part[content_length_pos] == ' ' || headers_part[content_length_pos] == '\t'))
                        content_length_pos++;

                    size_t end_pos = headers_part.find("\r\n", content_length_pos);
                    if (end_pos != std::string::npos)
                    {
                        std::string value = headers_part.substr(content_length_pos, end_pos - content_length_pos);
                        content_length = std::strtoul(value.c_str(), NULL, 10);
                        has_content_length = true;
                    }
                }
                bool request_complete = false;
                if (is_chunked)
                {
                    size_t end_of_chunks = recv_buffers[fd].find("0\r\n\r\n", header_end + 4);
                    if (end_of_chunks != std::string::npos)
                        request_complete = true;
                }
                else if (has_content_length)
                {
                    size_t total_expected = header_end + 4 + content_length;
                    if (recv_buffers[fd].size() >= total_expected)
                        request_complete = true;
                }
                else
                {
                    request_complete = true;
                }

                if (!request_complete)
                    continue;

                std::string full_request;
                if (is_chunked)
                {
                    size_t end_pos = recv_buffers[fd].find("0\r\n\r\n", header_end + 4);
                    full_request = recv_buffers[fd].substr(0, end_pos + 5);
                    recv_buffers[fd].erase(0, end_pos + 5);
                }
                else if (has_content_length)
                {
                    size_t total_expected = header_end + 4 + content_length;
                    full_request = recv_buffers[fd].substr(0, total_expected);
                    recv_buffers[fd].erase(0, total_expected);
                }
                else
                {
                    full_request = recv_buffers[fd].substr(0, header_end + 4);
                    recv_buffers[fd].erase(0, header_end + 4);
                }
                HttpRequest request;
                request.parse(full_request);
                Response response;

                if (!request.isValid())
                {
                    std::cerr << "Request not valid from fd=" << fd << std::endl;
                    response.setStatus(request.getErrorCode());
                    response.addHeader("Content-Type", "text/html");
                    response.setBody("<html><body><h1>Error " + 
                                intToString(request.getErrorCode()) + 
                                "</h1><p>" + request.getErrorMessage() + "</p></body></html>");
                    send_buffers[fd] = response.generate();
                    fds[i].events = POLLOUT;
                    recv_buffers.erase(fd);
                    continue;
                }
                std::cout << "✅ Request complete from fd=" << fd
                        << " (" << full_request.size() << " bytes)" << std::endl;
                bool is_cgi = false;
                std::string path = request.getPath();
                if (request.getPath().find("/cgi-bin/") != std::string::npos)
                {
                    //here TODO : cgi handler for a good guy
                    //recv_buffers[fd].erase(fd);
                    is_cgi = true;

                }
                if (path.find(".py") != std::string::npos ||
                        path.find(".php") != std::string::npos ||
                            path.find(".cgi") != std::string::npos)
                {
                    is_cgi = true;
                }
                if (is_cgi)
                {
                    std::cout << "CGI request : TODO == cgi handler" << std::endl;
                    // i dont think i need to add a response with an error etc..
                    // if i should do it, tell me 
                }
                else if (request.getMethod() == "GET")
                    response = FileHandler::handle(request, "./www");
                else if (request.getMethod() == "POST")
                    continue;
                    //response =UploadHandler::handle(request, "./uploads");
                else if (request.getMethod() == "DELETE")
                {
                    //abkhefif: i will do this and post
                    continue;
                }
                else
                {
                    response = Response();
                    response.setStatus(405);
                    response.addHeader("Content-Type", "text/html");
                    response.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
                }
                send_buffers[fd] = response.generate();
                fds[i].events = POLLOUT;
                recv_buffers.erase(fd);
            }

            // if (request.find("/cgi-bin/") != std::string::npos)
            // {
            //     int pipefd[2];
            //     if (pipe(pipefd) < 0)
            //     {
            //         std::cerr << "pipe error" << std::endl;
            //         continue;
            //     }

            //     int pid = fork();
            //     if (pid == 0)
            //     {
            //         close(pipefd[0]);
            //         dup2(pipefd[1], STDOUT_FILENO);
            //         close(pipefd[1]);
            //         char *args[] = {(char *)"/usr/bin/env", (char *)"echo", (char *)"CGI OUTPUT", NULL};
            //         execve("/usr/bin/env", args, NULL);
            //         exit(1);
            //     }
            //     else if (pid > 0)
            //     {
            //         close(pipefd[1]);
            //         struct pollfd cgi_poll = {pipefd[0], POLLIN, 0};
            //         fds.push_back(cgi_poll);
            //         cgi_pipes[pipefd[0]] = fd;
            //     }
            //     else
            //     {
            //         std::cerr << "fork error" << std::endl;
            //     }
            // }
            // else
            // {
            //     std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!";
            //     send_buffers[fd] = response;
            //     fds[i].events |= POLLOUT;
            // }
            // recv_buffers[fd].clear();
            if (fds[i].revents & POLLOUT)
            {
                if (send_buffers.find(fd) != send_buffers.end())
                {
                    std::string &buf = send_buffers[fd];
                    int bytes_sent = send(fd, buf.c_str(), buf.size(), 0);
                    if (bytes_sent < 0)
                    {
                        if (errno != EAGAIN && errno != EWOULDBLOCK)
                        {
                            std::cerr << "send failed, closing client" << std::endl;
                            close(fd);
                            send_buffers.erase(fd);
                            recv_buffers.erase(fd);
                            fds.erase(fds.begin() + i);
                            --i;
                            continue;
                        }
                        continue;
                    }
                    else if (bytes_sent < (int)buf.size())
                    {
                        buf.erase(0, bytes_sent);
                    }
                    else
                    {
                        buf.clear();
                        fds[i].events &= ~POLLOUT;
                        continue;
                    }
                }
            }
        }
    }
    close(listen_fd);
    return 0;
}