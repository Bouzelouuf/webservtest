#include "Webserv.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "FileHandler.hpp"
#include "CGI_handler.hpp"
#include "UploadHandler.hpp"

std::string intToString(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

// Funzione helper: dice che tipo di fd è
enum FdType {
    FD_LISTEN,      // Socket che accetta connessioni
    FD_CLIENT,      // Socket cliente HTTP
    FD_CGI_INPUT,   // Pipe per scrivere VERSO CGI (stdin dello script)
    FD_CGI_OUTPUT   // Pipe per leggere DA CGI (stdout dello script)
};

FdType getFdType(int fd, int listen_fd, std::map<int, CgiProcess> &cgi_map)
{
    if (fd == listen_fd)
        return FD_LISTEN;
    
    // È l'output di un CGI?
    if (cgi_map.count(fd) > 0)
        return FD_CGI_OUTPUT;
    
    // È l'input di un CGI?
    for (std::map<int, CgiProcess>::iterator it = cgi_map.begin(); it != cgi_map.end(); ++it)
    {
        if (it->second.pipe_in == fd)
            return FD_CGI_INPUT;
    }
    
    // Altrimenti è un client normale
    return FD_CLIENT;
}

int main()
{
    std::cout << "Initializing Webserv..." << std::endl;

    // ========== SETUP SOCKET ==========
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        std::cerr << "socket error";
        return 1;
    }

    int option = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
    {
        std::cerr << "setsockopt error";
        return 1;
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "bind error";
        return 1;
    }
    
    if (listen(listen_fd, SOMAXCONN) < 0)
    {
        std::cerr << "listen error";
        return 1;
    }
    
    // Rendi non-bloccante
    int flags = fcntl(listen_fd, F_GETFL, 0);
    fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK);

    std::cout << "Webserv is running on port " << PORT << std::endl;

    // ========== STRUTTURE DATI ==========
    std::vector<struct pollfd> fds;
    std::map<int, std::string> send_buffers;    // fd → dati da inviare
    std::map<int, std::string> recv_buffers;    // fd → dati ricevuti

    // Mappa CGI: pipe_output → struttura CGI
    std::map<int, CgiProcess> cgi_map;
    // Mappa: pipe_output → fd_client (per sapere a chi inviare la risposta)
    std::map<int, int> cgi_client_map;

    // Aggiungi listen_fd a poll
    struct pollfd listen_poll = {listen_fd, POLLIN, 0};
    fds.push_back(listen_poll);

    // ========== LOOP PRINCIPALE ==========
    while (1)
    {
        int ready = poll(fds.data(), fds.size(), -1);
        if (ready < 0)
        {
            std::cerr << "poll error" << std::endl;
            break;
        }

        // Itera su tutti i fd monitorati
        for (size_t i = 0; i < fds.size(); i++)
        {
            int fd = fds[i].fd;
            int revents = fds[i].revents;

            // ==========================================
            // GESTIONE ERRORI
            // ==========================================
            if (revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                std::cerr << "Error on fd " << fd << ", closing..." << std::endl;
                
                FdType type = getFdType(fd, listen_fd, cgi_map);
                
                if (type == FD_CGI_OUTPUT)
                {
                    // Se è output CGI, chiudi anche input
                    CgiProcess &cgi = cgi_map[fd];
                    if (!cgi.stdin_closed)
                        close(cgi.pipe_in);
                    cgi_map.erase(fd);
                    cgi_client_map.erase(fd);
                }
                else if (type == FD_CGI_INPUT)
                {
                    // Se è input CGI, trova e chiudi anche output
                    for (std::map<int, CgiProcess>::iterator it = cgi_map.begin(); it != cgi_map.end(); ++it)
                    {
                        if (it->second.pipe_in == fd)
                        {
                            close(it->second.pipe_out);
                            int pipe_out = it->first;
                            cgi_map.erase(it);
                            cgi_client_map.erase(pipe_out);
                            break;
                        }
                    }
                }
                
                close(fd);
                send_buffers.erase(fd);
                recv_buffers.erase(fd);
                fds.erase(fds.begin() + i);
                i--; // Importante: compensa l'erase
                continue;
            }

            // ==========================================
            // CASO 1: NUOVO CLIENT (POLLIN su listen_fd)
            // ==========================================
            if (fd == listen_fd && (revents & POLLIN))
            {
                int client_fd = accept(listen_fd, NULL, NULL);
                if (client_fd < 0)
                {
                    std::cerr << "accept error" << std::endl;
                    continue;
                }
                
                // Rendi il client non-bloccante
                fcntl(client_fd, F_SETFL, O_NONBLOCK);
                
                // Aggiungi a poll per monitorare lettura
                struct pollfd client_poll = {client_fd, POLLIN, 0};
                fds.push_back(client_poll);
                
                std::cout << "New client connected: fd=" << client_fd << std::endl;
                continue;
            }

            // ==========================================
            // CASO 2: LETTURA DA CGI OUTPUT (POLLIN)
            // ==========================================
            if ((revents & POLLIN) && cgi_map.count(fd) > 0)
            {
                CgiProcess &cgi = cgi_map[fd];
                
                // Leggi dati dallo stdout del CGI
                char buf[1024];
                ssize_t n = read(fd, buf, sizeof(buf));
                
                if (n > 0)
                {
                    cgi.read_buffer.append(buf, n);
                    std::cout << "Read " << n << " bytes from CGI" << std::endl;
                }
                else if (n == 0)
                {
                    std::cout << "CGI closed output pipe" << std::endl;
                }
                
                // Trasferisci output al buffer del client
                int client_fd = cgi_client_map[fd];
                send_buffers[client_fd] += cgi.read_buffer;
                cgi.read_buffer.clear();
                
                // Abilita scrittura verso client
                for (size_t j = 0; j < fds.size(); j++)
                {
                    if (fds[j].fd == client_fd)
                    {
                        fds[j].events |= POLLOUT;
                        break;
                    }
                }
                
                // Controlla se il processo CGI è terminato
                int status;
                pid_t result = waitpid(cgi.pid, &status, WNOHANG);
                if (result > 0)
                {
                    std::cout << "CGI process finished" << std::endl;
                    
                    // Chiudi pipe
                    close(fd);
                    if (!cgi.stdin_closed)
                        close(cgi.pipe_in);
                    
                    // Rimuovi da mappe
                    cgi_map.erase(fd);
                    cgi_client_map.erase(fd);
                    
                    // Rimuovi da fds
                    fds.erase(fds.begin() + i);
                    i--;
                }
                
                continue;
            }

            // ==========================================
            // CASO 3: SCRITTURA VERSO CGI INPUT (POLLOUT)
            // ==========================================
            if (revents & POLLOUT)
            {
                FdType type = getFdType(fd, listen_fd, cgi_map);
                
                if (type == FD_CGI_INPUT)
                {
                    // Trova il CgiProcess associato
                    CgiProcess *cgi_ptr = NULL;
                    for (std::map<int, CgiProcess>::iterator it = cgi_map.begin(); it != cgi_map.end(); ++it)
                    {
                        if (it->second.pipe_in == fd)
                        {
                            cgi_ptr = &(it->second);
                            break;
                        }
                    }
                    
                    if (cgi_ptr && !cgi_ptr->stdin_closed && !cgi_ptr->write_buffer.empty())
                    {
                        // Scrivi dati verso stdin del CGI
                        ssize_t written = write(fd, cgi_ptr->write_buffer.c_str(), cgi_ptr->write_buffer.size());
                        
                        if (written > 0)
                        {
                            std::cout << "Wrote " << written << " bytes to CGI" << std::endl;
                            cgi_ptr->write_buffer.erase(0, written);
                        }
                        
                        // Se ho finito di scrivere, chiudi stdin
                        if (cgi_ptr->write_buffer.empty())
                        {
                            close(fd);
                            cgi_ptr->stdin_closed = true;
                            std::cout << "CGI stdin closed" << std::endl;
                            
                            // Rimuovi POLLOUT da questo fd
                            fds[i].events &= ~POLLOUT;
                        }
                    }
                    
                    continue; // Vai al prossimo fd
                }
            }

            // ==========================================
            // CASO 4: LETTURA DA CLIENT (POLLIN)
            // ==========================================
            if (revents & POLLIN)
            {
                FdType type = getFdType(fd, listen_fd, cgi_map);
                
                if (type == FD_CLIENT)
                {
                    char buf[1024];
                    int bytes_received = recv(fd, buf, sizeof(buf) - 1, 0);
                    
                    if (bytes_received <= 0)
                    {
                        // Client disconnesso
                        std::cout << "Client fd=" << fd << " disconnected" << std::endl;
                        close(fd);
                        send_buffers.erase(fd);
                        recv_buffers.erase(fd);
                        fds.erase(fds.begin() + i);
                        i--;
                        continue;
                    }
                    
                    buf[bytes_received] = '\0';
                    recv_buffers[fd] += std::string(buf, bytes_received);
                    
                    // Controlla se la richiesta è completa
                    size_t header_end = recv_buffers[fd].find("\r\n\r\n");
                    if (header_end == std::string::npos)
                        continue; // Header incompleto, aspetta altri dati
                    
                    // Parsing Content-Length e Transfer-Encoding
                    std::string headers_part = recv_buffers[fd].substr(0, header_end + 4);
                    size_t content_length = 0;
                    bool has_content_length = false;
                    bool is_chunked = false;
                    
                    size_t cl_pos = headers_part.find("Content-Length:");
                    if (cl_pos != std::string::npos)
                    {
                        cl_pos += 15;
                        while (cl_pos < headers_part.size() && (headers_part[cl_pos] == ' ' || headers_part[cl_pos] == '\t'))
                            cl_pos++;
                        size_t end_pos = headers_part.find("\r\n", cl_pos);
                        if (end_pos != std::string::npos)
                        {
                            std::string value = headers_part.substr(cl_pos, end_pos - cl_pos);
                            content_length = std::strtoul(value.c_str(), NULL, 10);
                            has_content_length = true;
                        }
                    }
                    
                    size_t te_pos = headers_part.find("Transfer-Encoding:");
                    if (te_pos != std::string::npos)
                    {
                        size_t line_end = headers_part.find("\r\n", te_pos);
                        std::string value = headers_part.substr(te_pos, line_end - te_pos);
                        if (value.find("chunked") != std::string::npos)
                            is_chunked = true;
                    }
                    
                    // Verifica se richiesta completa
                    bool request_complete = false;
                    if (is_chunked)
                    {
                        if (recv_buffers[fd].find("0\r\n\r\n", header_end + 4) != std::string::npos)
                            request_complete = true;
                    }
                    else if (has_content_length)
                    {
                        if (recv_buffers[fd].size() >= header_end + 4 + content_length)
                            request_complete = true;
                    }
                    else
                    {
                        request_complete = true;
                    }
                    
                    if (!request_complete)
                        continue;
                    
                    // Estrai richiesta completa
                    std::string full_request;
                    if (is_chunked)
                    {
                        size_t end = recv_buffers[fd].find("0\r\n\r\n", header_end + 4);
                        full_request = recv_buffers[fd].substr(0, end + 5);
                        recv_buffers[fd].erase(0, end + 5);
                    }
                    else if (has_content_length)
                    {
                        size_t total = header_end + 4 + content_length;
                        full_request = recv_buffers[fd].substr(0, total);
                        recv_buffers[fd].erase(0, total);
                    }
                    else
                    {
                        full_request = recv_buffers[fd].substr(0, header_end + 4);
                        recv_buffers[fd].erase(0, header_end + 4);
                    }
                    
                    // PARSING RICHIESTA HTTP
                    HttpRequest request;
                    request.parse(full_request);
                    Response response;
                    
                    if (!request.isValid())
                    {
                        response.setStatus(request.getErrorCode());
                        response.addHeader("Content-Type", "text/html");
                        response.setBody("<html><body><h1>Error " + intToString(request.getErrorCode()) + 
                                       "</h1><p>" + request.getErrorMessage() + "</p></body></html>");
                        send_buffers[fd] = response.generate();
                        fds[i].events = POLLOUT;
                        continue;
                    }
                    
                    std::cout << "Request: " << request.getMethod() << " " << request.getPath() << std::endl;
                    
                    // Determina se è CGI
                    std::string path = request.getPath();
                    bool is_cgi = (path.find("/cgi-bin/") != std::string::npos ||
                                   path.find(".py") != std::string::npos ||
                                   path.find(".php") != std::string::npos ||
                                   path.find(".cgi") != std::string::npos);
                    
                    if (is_cgi)
                    {
                        // GESTIONE CGI
                        try
                        {
                            CgiProcess cgi = CGIHandler::spawnCgi(request, path, "./www");
                            
                            // Registra nelle mappe
                            cgi_map[cgi.pipe_out] = cgi;
                            cgi_client_map[cgi.pipe_out] = fd;
                            
                            // Aggiungi pipe_out per leggere output
                            struct pollfd cgi_out = {cgi.pipe_out, POLLIN, 0};
                            fds.push_back(cgi_out);
                            
                            // Se c'è body da inviare, aggiungi pipe_in per scrittura
                            if (!cgi.write_buffer.empty())
                            {
                                struct pollfd cgi_in = {cgi.pipe_in, POLLOUT, 0};
                                fds.push_back(cgi_in);
                            }
                            else
                            {
                                // Nessun body, chiudi subito stdin
                                close(cgi.pipe_in);
                                cgi_map[cgi.pipe_out].stdin_closed = true;
                            }
                            
                            std::cout << "CGI spawned: pipe_in=" << cgi.pipe_in 
                                    << ", pipe_out=" << cgi.pipe_out << std::endl;
                        }
                        catch (const std::exception &e)
                        {
                            response.setStatus(500);
                            response.addHeader("Content-Type", "text/html");
                            response.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
                            send_buffers[fd] = response.generate();
                            fds[i].events = POLLOUT;
                        }
                    }
                    else if (request.getMethod() == "GET")
                    {
                        // FILE STATICO
                        response = FileHandler::handle(request, "./www");
                        send_buffers[fd] = response.generate();
                        fds[i].events = POLLOUT;
                    }
                    else if (request.getMethod() == "POST")
                    {
                        response = UploadHandler::handle(request, "./uploads");
                        send_buffers[fd] = response.generate();
                        fds[i].events = POLLOUT;
                    }
                    else
                    {
                        response.setStatus(405);
                        response.addHeader("Content-Type", "text/html");
                        response.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
                        send_buffers[fd] = response.generate();
                        fds[i].events = POLLOUT;
                    }
                    
                    continue;
                }
            }

            // ==========================================
            // CASO 5: SCRITTURA VERSO CLIENT (POLLOUT)
            // ==========================================
            if (revents & POLLOUT)
            {
                FdType type = getFdType(fd, listen_fd, cgi_map);
                
                if (type == FD_CLIENT)
                {
                    if (send_buffers.find(fd) != send_buffers.end())
                    {
                        std::string &buf = send_buffers[fd];
                        int bytes_sent = send(fd, buf.c_str(), buf.size(), 0);
                        
                        if (bytes_sent < 0)
                        {
                            if (errno != EAGAIN && errno != EWOULDBLOCK)
                            {
                                std::cerr << "send error on fd " << fd << std::endl;
                                close(fd);
                                send_buffers.erase(fd);
                                recv_buffers.erase(fd);
                                fds.erase(fds.begin() + i);
                                i--;
                            }
                            continue;
                        }
                        
                        if (bytes_sent < (int)buf.size())
                        {
                            // Invio parziale
                            buf.erase(0, bytes_sent);
                        }
                        else
                        {
                            // Tutto inviato
                            buf.clear();
                            fds[i].events &= ~POLLOUT; // Disabilita POLLOUT
                        }
                    }
                    
                    continue;
                }
            }
        } // Fine loop su fds
        
    } // Fine while(1)

    close(listen_fd);
    return 0;
}