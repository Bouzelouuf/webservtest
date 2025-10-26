# WebServ - HTTP Server in C++98

## Features

- **HTTP/1.1 Protocol Support**
  - GET, POST, DELETE methods
  - Custom error pages
  - Accurate HTTP status codes

- **Non-blocking I/O**
  - Single poll() call for all I/O operations
  - Handles multiple clients simultaneously
  - Client timeout management

- **Configuration System**
  - Multiple server instances on different ports
  - Location-based routing
  - Custom error pages per server
  - File upload configuration
  - CGI script support

- **Static File Serving**
  - MIME type detection
  - Directory listing (autoindex)
  - Index file support

- **File Upload**
  - POST method for file uploads
  - Multipart form data support
  - Configurable upload directories

- **CGI Support**
  - Execute Python, PHP, and other scripts
  - Environment variable setup
  - Request body forwarding

## Building

```bash
make
```

## Running

```bash
./webserv [config_file]
```

If no configuration file is specified, it defaults to `config/default.conf`.

## Configuration

Example configuration file:

```
server {
    listen 8080;
    host 0.0.0.0;
    server_name localhost;

    error_page 404 www/errors/404.html;
    client_max_body_size 10485760;

    location / {
        root www;
        index index.html;
        allow_methods GET POST DELETE;
        autoindex off;
    }

    location /uploads {
        root www;
        allow_methods GET POST DELETE;
        autoindex on;
        upload_path www/uploads;
    }

    location /cgi-bin {
        root www;
        allow_methods GET POST;
        cgi_extension .py;
        cgi_path /usr/bin/python3;
    }
}
```

## Testing

### Test with browser
Open http://localhost:8080 in your browser.

### Test with curl

```bash
# GET request
curl http://localhost:8080/

# POST file upload
curl -X POST -F "file=@test.txt" http://localhost:8080/uploads

# DELETE request
curl -X DELETE http://localhost:8080/uploads/test.txt
```

### Test with telnet

```bash
telnet localhost 8080
GET / HTTP/1.1
Host: localhost

```

## Project Structure

```
.
├── Makefile
├── config/
│   └── default.conf
├── include/
│   ├── Client.hpp
│   ├── Config.hpp
│   ├── CGI.hpp
│   ├── HttpRequest.hpp
│   ├── HttpResponse.hpp
│   ├── LocationConfig.hpp
│   ├── Server.hpp
│   ├── ServerConfig.hpp
│   ├── Utils.hpp
│   └── WebServer.hpp
├── src/
│   ├── Client.cpp
│   ├── Config.cpp
│   ├── CGI.cpp
│   ├── HttpRequest.cpp
│   ├── HttpResponse.cpp
│   ├── LocationConfig.cpp
│   ├── Server.cpp
│   ├── ServerConfig.cpp
│   ├── Utils.cpp
│   ├── WebServer.cpp
│   └── main.cpp
└── www/
    ├── index.html
    ├── about.html
    ├── contact.html
    ├── errors/
    │   ├── 404.html
    │   └── 500.html
    ├── cgi-bin/
    │   └── test.py
    └── uploads/
```

## Requirements

- C++ compiler with C++98 support
- POSIX-compliant operating system (Linux, macOS)
- Python 3 (for CGI scripts)

## Cleaning

```bash
make clean   # Remove object files
make fclean  # Remove object files and executable
make re      # Rebuild everything
