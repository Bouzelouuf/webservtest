# Quick Start Guide

This will compile all source files and create the `webserv` executable.

## Running the Server

### With default configuration
```bash
./webserv
```
This will use the default configuration file at `config/default.conf`.

### With custom configuration
```bash
./webserv path/to/config.conf
```

## Default Server Setup

The default configuration starts two servers:

1. **Main Server** - http://localhost:8080
   - Static file serving
   - File upload support (/uploads)
   - CGI support (/cgi-bin)
   - Custom error pages

2. **Alternative Server** - http://localhost:8081
   - Simple static file serving
   - Different document root (www2)

## Testing the Server

### 1. Start the Server
```bash
./webserv
```

You should see:
```
Server listening on 0.0.0.0:8080
Server listening on 0.0.0.0:8081
WebServer started
```

### 2. Test with Browser

Open your browser and visit:
- http://localhost:8080 - Main server home page
- http://localhost:8081 - Alternative server
- http://localhost:8080/about.html - About page
- http://localhost:8080/uploads - Upload directory (autoindex enabled)

### 3. Test with curl

#### GET Request
```bash
curl http://localhost:8080/
```

#### POST File Upload
```bash
echo "Test content" > test.txt
curl -X POST -F "file=@test.txt" http://localhost:8080/uploads
```

#### View Uploaded File
```bash
curl http://localhost:8080/uploads/test.txt
```

#### DELETE File
```bash
curl -X DELETE http://localhost:8080/uploads/test.txt
```

#### Test CGI
```bash
curl http://localhost:8080/cgi-bin/test.py
```

### 4. Test with telnet

```bash
telnet localhost 8080
```

Then type:
```
GET / HTTP/1.1
Host: localhost

```
(Press Enter twice after the Host line)

### 5. Run Automated Tests

```bash
./test.sh
```

This will run a series of tests including:
- GET requests
- POST file uploads
- DELETE operations
- CGI script execution
- 404 error pages

## Stopping the Server

Press `Ctrl+C` to gracefully shutdown the server.

## Makefile Targets

- `make` or `make all` - Build the project
- `make clean` - Remove object files
- `make fclean` - Remove object files and executable
- `make re` - Rebuild everything from scratch

## Troubleshooting

### Port Already in Use
If you get a "Failed to bind socket" error:
```bash
# Check what's using the port
lsof -i :8080

# Either kill that process or change the port in config/default.conf
```

### Permission Denied for CGI
Make sure the CGI script is executable:
```bash
chmod +x www/cgi-bin/test.py
```

### Upload Directory Not Writable
Ensure the uploads directory has write permissions:
```bash
chmod 755 www/uploads
```

## Configuration Tips

1. Edit `config/default.conf` to customize:
   - Ports to listen on
   - Document roots
   - Upload paths
   - Error pages
   - Allowed HTTP methods
   - CGI settings

2. You can create multiple server blocks for different ports

3. Use location blocks to configure different behaviors for different paths

## Next Steps

- Read `docs/CONFIGURATION.md` for detailed configuration options
- Read `docs/IMPLEMENTATION.md` for technical details
- Customize the HTML files in `www/` directory
- Add your own CGI scripts to `www/cgi-bin/`
- Test with stress testing tools like `siege` or `ab`

## Performance Testing

### Apache Bench
```bash
ab -n 1000 -c 10 http://localhost:8080/
```

### Siege
```bash
siege -c 10 -t 30s http://localhost:8080/
```
