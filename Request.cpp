#include "Request.hpp"

HttpRequest::HttpRequest(): content_length(0), has_content_length(false),is_chunked(false){}

void HttpRequest::parse(std::string request)
{
    //on converti en stream la str pour eviter direct les 
    //espace entre les infos
    std::istringstream stream(request);
    std::string line;

    //on va lire la premier ligne
    std::getline(stream, line);
    //std::cout << "DDEBUG line='" << line << "'" << std::endl;

    if (!line.empty() && line[line.size() - 1] == '\r')
    {
        //on enleve le \r qui termine une ligne de HttpRequest
        line.erase(line.size() - 1);
    }
    
    std::istringstream line_stream(line);
    //creation de lobjet linestream qui lit line 
    line_stream >> method >> uri >> version;
    // >> saute automatiquement les espaces , mais stock ce que tu trouve avant dant >> method
    // puis dans >> uri puis dans version
    validateMethod();
    validateVersion();
    validateUri();
    parseQueryString();
    while (std::getline(stream, line))
    {
        //pour les headers maintenant
        //supprimer les \r dabbord si ligne pas empty
        if (line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            break;
        size_t sep_pos = line.find(':');
        if (sep_pos == std::string::npos)
            continue;
        //npos c une valeur retourner pour dire non found
        //car find retourn tj une valeur
        std::string key = line.substr(0, sep_pos);
        std::string value = line.substr(sep_pos + 1);
        size_t start = value.find_first_not_of(" \t\r\n");
        //trouve la pos du premier char qui n'est pas espace ou tab
        // pour ne pas remplie les value dans headers avec espace / tab
        if (start != std::string::npos)
            value = value.substr(start);
        else
            value = "";
        size_t end = value.find_last_not_of(" \t\r\n");
        if (end != std::string::npos)
            value = value.substr(0, end + 1);
        else
            value = "";
        key = toLower(key);
        headers[key] = value;
    }
    validateHost();
    std::string transfer_encoding = getHeader("transfer-encoding");
    if (transfer_encoding == "chunked" && hasContentLength())
        throw std::runtime_error("400 Bad Request: Content-Length with Transfer-Encoding");
    if (transfer_encoding == "chunked" )
    {
        is_chunked = true;
        size_t body_start = request.find("\r\n\r\n");
        if (body_start != std::string::npos)
        {
            body_start += 4;
            std::string raw_body = request.substr(body_start);
            body = dechunkBody(raw_body);
        }
    }
    else if (hasContentLength())
    {
        size_t body_start = request.find("\r\n\r\n");
        if (body_start != std::string::npos)
        {
            body_start += 4;
            body = request.substr(body_start);
        }
        validateContentLength();
        parsePostBody();
    }
    parseCookies();
}

void HttpRequest::validateContentLength()
{
    std::map<std::string, std::string>::iterator it = headers.find("content-length");
    if (it != headers.end())
    {
        has_content_length = true;
        std::istringstream input_strring_stream(it->second);
        //devrait sappeler iss ( convention)
        // pour convertir content length en valeur int

        
        //stock la valeur trouver it (length) converti en stream dans input_strring_stream 
        //
        if (!(input_strring_stream >> content_length))
        {
            std::cerr << "Error: Invalid Content-Length format" << std::endl;
            throw std::runtime_error("400 Bad HttpRequest: Invalid Content-Length");
        }
        if (content_length > MAX_BODY_SIZE)
        {
            std::cerr   << "Error: Content-Length too large (" << content_length
                        << " > " << MAX_BODY_SIZE << ")" << std::endl;
            throw std::runtime_error("413 Payload Too Large");
        }
        if (body.length() != content_length)
        {
            std::cerr << "Error: Content-Length mismatch!" << std::endl;
            std::cerr << "  Declared in header: " << content_length << std::endl;
            std::cerr << "  Actual body length: " << body.length() << std::endl;
            throw std::runtime_error("400 Bad HttpRequest: Content-Length mismatch");
        }
        std::cout << "Content-Length validated: " << content_length << " bytes" << std::endl;
    }
    else
    {
        has_content_length = false;
        content_length = 0;
        if (body.length() > MAX_BODY_SIZE)
        {
            std::cerr   << "Error: Body too large without Content-Length(" << body.length()
                        << " > " << MAX_BODY_SIZE << ")" << std::endl;
            throw std::runtime_error("413 Payload Too Large");
        }
        if ((method == "POST" || method == "PUT") && !body.empty())
        {
            std::cerr << "Warming: POST/PUT HttpRequest without Content-Length" << std::endl;
            // throw une erreur ???
        }
    }
}

std::string HttpRequest::getMethod()const
{
    return method;
}

std::string HttpRequest::getUri()const
{
    return uri;
}

std::string HttpRequest::getVersion()const
{
    return version;
}

std::string HttpRequest::getBody()const
{
    return body;
}

std::string HttpRequest::getHeader(std::string param)const
{
    std::string key_lower = toLower(param);
    std::map<std::string, std::string>::const_iterator it = headers.find(key_lower);
    if (it != headers.end())
        return it->second;
    return "";
}

void HttpRequest::displayHeaders()const
{
    std::map<std::string, std::string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it)
    {
        std::cout << it->first << ": " << it->second << std::endl;
    }
}

size_t HttpRequest::getContentLength() const
{
    return content_length;
}

bool HttpRequest::hasContentLength()const
{
   return has_content_length; 
}

std::string HttpRequest::toLower(const std::string& str)const
{
    std::string cont_len_lower = str;
    for (size_t i = 0; i < str.size(); i++)
    {
        if (cont_len_lower[i] >= 'A' && cont_len_lower[i] <= 'Z')
            cont_len_lower[i] = cont_len_lower[i] + 32;
    }
    return cont_len_lower;
}

void HttpRequest::validateMethod()
{
    if (method.empty())
    {
        std::cerr << "Error: Method is empty" << std::endl;
        throw std::runtime_error("400 Bad HttpRequest: No method specified");
    }
    if (!isMethodValid(method))
    {
        std::cerr << "Error: Invalid HTTP method: " << method << std::endl; 
        throw std::runtime_error("405 Method Not Allowed");
    }
    std::cout << "Method validated: " << method << std::endl;
}

bool HttpRequest::isMethodValid(const std::string& method)const
{
    if (method == "GET" || method == "POST" || method == "DELETE")
        return true;
    //je dois gerer plus ???
    return false;
}

void HttpRequest::validateVersion()
{
    if (version.empty())
    {
        std::cerr << "Error: HTTP version is empty" << std::endl;
        throw std::runtime_error("400 Bad HttpRequest: No HTTP version specified");
    }
    if (version != "HTTP/1.0" && version != "HTTP/1.1")
    {
        std::cerr << "Error: Unsupported HTTP Version: " << version << std::endl;
        throw std::runtime_error("505 HTTP Version Not Supported");
    }
    std::cout << "Version validated" << std::endl;
}

void HttpRequest::validateUri()
{
    const size_t max_uri_len = 8192;
    if (uri.empty())
    {
        std::cerr << "Error: URI is empty" << std::endl;
        throw std::runtime_error("400 Bad HttpRequest: No URI specified");
    }
    if (uri.size() > max_uri_len)
    {
        std::cerr   << "Error: URI too long (" << uri.length() 
                    << " > " << max_uri_len <<std::endl;
        throw std::runtime_error("414 URI Too long");
    }
    if (uri[0] != '/')
        {
            std::cerr << "Error: URI must start with '/': " << uri << std::endl;
            throw std::runtime_error("400 Bad HttpRequest: Invalid URI format");
        }
    std::cout << "URI validated: " << uri << std::endl;

}

void HttpRequest::validateHost()
{
    if (version == "HTTP/1.1")
    {
        std::map<std::string, std::string>::iterator it = headers.find("host");
        if (it == headers.end())
        //si host nexist pas
        {
            std::cerr << "Error: Host header missing (required for HTTP/1.1)" << std::endl;
            throw std::runtime_error("400 Bad HttpRequest: Host header required");
        }
        if (it->second.empty())
        {
            std::cerr << "Error: Host header is empty" << std::endl;
            throw std::runtime_error("400 Bad HttpRequest: Host header cannot be empty");
        }
        std::cout << "Host validated: " << it->second << std::endl;
    }
    else
    {
        std::cout << "Host validation skipped(HTTP/1.0)" << std::endl;
    }
}

void HttpRequest::parseQueryString()
{
    size_t question_mark_pos = uri.find('?');
    //separation du path et de la partie query string
    if (question_mark_pos == std::string::npos)
    {
        //si pas de querry string alors path est le uri
        path = uri;
        return;
    }
    path = uri.substr(0, question_mark_pos);
    // on extrait tout avant le ? pour trouver le path
    std::string query_string = uri.substr(question_mark_pos + 1);
    //inverse tout apres le ? 
    std::istringstream stream(query_string);
    // on let met en stream pour avant de morceau en morceau
    std::string pair;
    while (std::getline(stream, pair, '&'))
    {
        // ici vu que c un stream on peut jump de & en & 
        size_t equal_pos = pair.find('=');
        //si on trouve un = donc on a un a ajouter  
        if (equal_pos != std::string::npos)
        {
            // ici on ajt le key et ensuit value 
            std::string key = pair.substr(0, equal_pos);
            std::string value = pair.substr(equal_pos + 1);
            key = urlDecode(key);
            value = urlDecode(value);
            query_params[key] = value;
            // et on ajout ca dans la map query params
        }
        else
        {
            std::string key = urlDecode(pair);
            query_params[pair] = "";
        }
    }
    std::cout << "query string parserd: " << query_params.size() << " parameters" << std::endl;
}

std::string HttpRequest::getPath()const
{
    return path;
}

std::string HttpRequest::getQueryParam(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it = query_params.find(key);
    if (it != query_params.end())
        return it->second;
    return "";
}

const std::map<std::string, std::string> &HttpRequest::getQueryParams()const
{
    return query_params;
}

std::string HttpRequest::urlDecode(const std::string& str)const
{
    // ex : "hello%20world"
    std::string result;
    // pour le result decoder
    for (size_t i = 0; i < str.length(); i++)
    {
        
        if (str[i] == '%' && i + 2 < str.length())
        {
            std::string hex = str.substr(i + 1, 2);
            char decoded_char;
            std::istringstream iss(hex);
            int value;
            if (iss >> std::hex >> value)
            {//on converti la valeur en 
                decoded_char = static_cast<char>(value);
                result += decoded_char;
                i += 2;
                //on saute le code 
            }
            else
                result += str[i];
        }
        else if (str[i] == '+')
            result += ' ';
        else
            result += str[i];
    }
    return result;
}

void HttpRequest::parsePostBody()
{
    if (method != "POST")
        return;
    std::string content_type = getHeader("content-type");
    if (content_type.empty())
        return;
    if (content_type.find("application/x-www-form-urlencoded") != std::string::npos)
    {
        std::istringstream stream(body);
        std::string pair;
        while(std::getline(stream, pair, '&'))
        {
            size_t equal_pos = pair.find('=');
            if (equal_pos != std::string::npos)
            {
                std::string key = pair.substr(0, equal_pos);
                std::string value = pair.substr(equal_pos + 1);
                key = urlDecode(key);
                value = urlDecode(value);
                post_params[key] = value;
            }
            else
           {
            std::string key = urlDecode(pair);
            post_params[key] = "";
           }
        }
        std::cout << "POST body parsed: " << post_params.size() << " parameters" << std::endl;
    }
    else if(content_type.find("application/json") != std::string::npos)
    {
        std::cout << "POST body (JSON): " << body.length() << " bytes" << std::endl;
    }

    else if (content_type.find("multipart/form-data") != std::string::npos)
    {
        // Extraire la boundary
        std::string boundary = extractBoundary(content_type);
        
        if (boundary.empty())
        {
            std::cerr << "Error: Multipart boundary not found in Content-Type" << std::endl;
            return;
        }
        
        std::cout << "Parsing multipart with boundary: '" << boundary << "'" << std::endl;
        
        // Parser le body multipart
        parseMultipartBody(boundary);
    }

    else
        std::cout << "POST body (unknow type): " << body.length() << " bytes" << std::endl;
}

std::string HttpRequest::getPostParam(const std::string &key)const
{
    std::map<std::string, std::string>::const_iterator it = post_params.find(key);
    if (it != post_params.end())
        return it->second;
    return "";
}

const std::map<std::string, std::string> &HttpRequest::getPostParams()const
{
    return post_params;
}

void HttpRequest::parseCookies()
{
    std::string cookie_header = getHeader("cookie");
    if (cookie_header.empty())
        return;
    std::istringstream stream(cookie_header);
    std::string pair;
    //decouper selon ';'
    while(std::getline(stream, pair, ';'))
    {
        size_t start = pair.find_first_not_of(" \t");
        if (start != std::string::npos)
            pair = pair.substr(start);
        size_t equal_pos = pair.find('=');
        if (equal_pos != std::string::npos)
        {
            std::string key = pair.substr(0, equal_pos);
            std::string value = pair.substr(equal_pos + 1);
            size_t end = value.find_last_not_of(" \t");
            if (end != std::string::npos)
                value= value.substr(0, end + 1);
            cookies[key] = value;
        }
    }
    if (!cookies.empty())
        std::cout << "Cookies parsed: " << cookies.size() << " cookies" << std::endl;
}

std::string HttpRequest::getCookie(const std::string &key)const
{
    std::map<std::string, std::string>::const_iterator it = cookies.find(key);
    if (it != cookies.end())
        return it->second;
    return "";
}

const std::map<std::string, std::string>& HttpRequest::getCookies()const
{
    return cookies;
}

std::string HttpRequest::extractBoundary(const std::string& content_type)const
{
    //cherche le boundary , le separator pour post un file
    // ex: "multipart/form-data; boundary=----WebKit"

    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos)
        return "";
    boundary_pos += 9;
    //jump le boundary

    size_t end_pos = content_type.find(';', boundary_pos);
    //ex1:"boundary=----WebKit; charset=utf-8"
    //ex2:: "boundary=----WebKit;"
    // si parametre apres, end pos = ';' 
    std::string boundary;
    if (end_pos != std::string::npos)
        boundary = content_type.substr(boundary_pos, end_pos - boundary_pos);
    else
        boundary = content_type.substr(boundary_pos);

    //enlever les space tab newline
    size_t start = boundary.find_first_not_of(" \t\r\n");
    if (start != std::string::npos)
        boundary = boundary.substr(start);
    size_t end = boundary.find_last_not_of(" \t\r\n");
    if (end != std::string::npos)
        boundary = boundary.substr(0, end + 1);
    return boundary;
}

std::string HttpRequest::extractValueFromHeader(const std::string& header, const std::string& key)const
{
    //on cherche key = "value" dans le header
    //ex: "name=\"avatar\"; filename=\"photo.jpg\""
    std::string search_key = key + "=";
    //string key a trouver
    size_t key_pos = header.find(search_key);
    if (key_pos == std::string::npos)
        return "";
    key_pos += search_key.length();
    if (key_pos < header.length() && header[key_pos] == '"')
    {
        key_pos++;
        size_t end_quote = header.find('"', key_pos);
        if (end_quote != std::string::npos)
            return header.substr(key_pos, end_quote - key_pos);
    }
    else
    {
        size_t end_pos = header.find_first_of(" \t\r\n", key_pos);
        if (end_pos != std::string::npos)
            return header.substr(key_pos, end_pos - key_pos);
        else
            return header.substr();
    }
    return "";
}

void HttpRequest::parseMultipartBody(const std::string& boundary)
{
    // on fait dabbord les separator
    std::string delimiter = "--" + boundary;
    std::string end_delimimiter = "--" + boundary + "--";
    //pos du premier sep
    size_t pos = body.find(delimiter);
    if (pos == std::string::npos)
    {
        std::cerr << "Error: Multipart boundary not found in body" << std::endl;
        return;
    }
    //on avance apres le premier boundary +  \r\n
    pos += delimiter.length();
    if (pos + 1 < body.length() && body[pos] == '\r' && body[pos + 1] == '\n')
        pos+= 2;
    else if (pos < body.length() && body[pos] == '\n')
        pos += 1;
    while (pos < body.length())
    {
        //on search le prochain boundary separator
        size_t next_boundary = body.find(delimiter, pos);
        if (next_boundary == std::string::npos)
            break;
        // extraire la partie entre body et next boundary
        std::string part = body.substr(pos, next_boundary - pos);
        //checker si ya pas de boundary ensuite (final)
        if (body.compare(next_boundary, end_delimimiter.length(), end_delimimiter ) == 0)
        {
            if (!part.empty())
                parseSinglePart(part);
            break;
        }
        parseSinglePart(part);
        pos = next_boundary + delimiter.length();
        if (pos + 1 < body.length() && body[pos] == '\r' && body[pos + 1] == '\n')
            pos += 2;
        else if (pos < body.length() && body[pos] == '\n')
            pos+= 1;
    }
    std::cout   << "Multipart body parsed: " << post_params.size() << " fields, "
                << uploaded_files.size() << " files" << std::endl;
}

void HttpRequest::parseSinglePart(const std::string& part)
{
    // une part a ce format :
    // Content-Disposition: form-data; name="username"
    // Content-Type: text/plain
    // 
    // john
    size_t separator = part.find("\r\n\r\n");
    size_t header_end = 0;
    size_t content_start = 0;
    if (separator != std::string::npos)
    {
        header_end = separator;
        content_start = separator + 4; // pour \r\n\r\n
    }
    else
    {
        separator = part.find("\n\n");
        if (separator != std::string::npos)
        {
            header_end = separator;
            content_start = separator + 2; // sep est \n\n cette fois
        }
        else
            return;
        //pas de sep trouver
    }

    std::string header_section = part.substr(0, header_end);
    std::string content = part.substr(content_start);

    if (content.length() >= 2 && content[content.length() - 2] == '\r' && content[content.length() - 1] == '\n')
        content = content.substr(0, content.length() - 2);
    else if (content.length() >= 1 && content[content.length() - 1] == '\n')
        content = content.substr(0, content.length() - 1);

    std::string name;
    std::string filename;
    std::string content_type;

    std::istringstream header_stream(header_section);
    std::string header_line;
    while(std::getline(header_stream, header_line))
    {
        if (!header_line.empty() && header_line[header_line.length() - 1] == '\r')
            header_line = header_line.substr(0, header_line.length() - 1);
        if (header_line.empty())
            continue;
        std::string lower_line = toLower(header_line);
        if (lower_line.find("content-disposition:") == 0)
        {
            name = extractValueFromHeader(header_line, "name");
            filename = extractValueFromHeader(header_line, "filename");
        }
        else if (lower_line.find("content-type") == 0)
        {
            size_t colon_pos = header_line.find(':');
            if (colon_pos != std::string::npos)
            {
                content_type = header_line.substr(colon_pos + 1);
                size_t start = content_type.find_first_not_of(" \t");
                if (start != std::string::npos)
                    content_type = content_type.substr(start);
            }
        }
   }
   if (name.empty())
   {
    std::cerr << "Warning: Multipart parti without name, skipping" << std::endl;
    return;
   }
   if (!filename.empty())
   {
    UploadedFile file;
    file.field_name = name;
    file.filename = filename;
    file.content_type = content_type;
    file.data = content;
    file.size = content.length();
    uploaded_files[name] = file;
    std::cout   << " File uploaded: field='" <<name << ", filename='" << filename
                << "', size=" << file.size << " bytes" << std::endl;
   }
   else
   {
    post_params[name] = content;
    std::cout << "  Field parsed: '" << name << "' = '" << content << "'" << std::endl;
   }
}

bool HttpRequest::hasFile(const std::string& field_name)const
{
    return uploaded_files.find(field_name) != uploaded_files.end();
}

UploadedFile HttpRequest::getFile(const std::string& field_name) const
{
    std::map<std::string, UploadedFile>::const_iterator it = uploaded_files.find(field_name);
    
    if (it != uploaded_files.end())
        return it->second;
    
    // Retourner un fichier vide si non trouvé
    return UploadedFile();
}

std::vector<std::string> HttpRequest::getFileNames() const
{
    std::vector<std::string> names;
    
    std::map<std::string, UploadedFile>::const_iterator it;
    for (it = uploaded_files.begin(); it != uploaded_files.end(); ++it)
    {
        names.push_back(it->first);
    }
    
    return names;
}

bool HttpRequest::isRegularFile(const std:: string &path)const
{
    struct stat path_stat;
    //struct pour savoir si c un dossier ou file valide
    // comme une fiche id pour un file
    if (stat(path.c_str(), &path_stat) != 0)
        //on pass le file dans la stuct qui se remplie avecc stat()
        return false;
        //stat va retourner -1 si error ( file exist pas)
    return S_ISREG(path_stat.st_mode);
    //st mode contient des bits qui encodent type + permission ( pas tres clair)
    //s isreg est une macro qui test les bits de st mode
    //true si file normal 
}

bool HttpRequest::isDirectory(const std::string &path_directory)const
{
    struct stat path_stat;

    if (stat(path_directory.c_str(), &path_stat) != 0)
        return false;
    return S_ISDIR(path_stat.st_mode);
    //c pareil a ca pret que la macro est s _ is dir
}

bool HttpRequest::fileExists(const std::string &path)const
{
    return (access(path.c_str(), F_OK) == 0);
    //access () test l'accessibliter dun file 
    // F_ok test son existence juste
    // 0 si file existe else -1 
}

size_t HttpRequest::getFileSize(const std::string &path)const
{
    struct stat path_stat;

    if (stat(path.c_str(), &path_stat) != 0)
        return 0; //erreur = taille 0

    return static_cast<size_t>(path_stat.st_size);
}

bool HttpRequest::canRead(const std::string &path)const
{
    return (access(path.c_str(), R_OK) == 0);
}

bool HttpRequest::canWrite(const std::string &path)const
{
    // pour upload delete
    return (access(path.c_str(), W_OK) == 0);
}

bool HttpRequest::canExecute(const std::string &path)const
{
    return (access(path.c_str(), X_OK) == 0);
    // pour differents script cgi
}

void HttpRequest::parsedChunkedBody(const std::string &raw_body)
{
    (void)raw_body;
    return;
}

std::string HttpRequest::dechunkBody(const std::string &chunked_data)
{
    std::string result;
    
    size_t pos = 0;
    while (true)
    {
        size_t line_end = chunked_data.find("\r\n", pos);
        if (line_end == std::string::npos)
            throw std::runtime_error("400 Bad Request: Invalid chunk");
        std::string size_line = chunked_data.substr(pos, line_end - pos);
        size_t chunk_size = strtol(size_line.c_str(), NULL, 16);
        // convert hexa a decimal
        if (chunk_size == 0)
            break;
        pos = line_end + 2; // on saute \r\n pour ligne suivante
        result += chunked_data.substr(pos, chunk_size);
        pos = pos + chunk_size + 2;
    }
    return result;
}

bool HttpRequest::isChunked()const
{
    return is_chunked;
}
