#include "UploadHandler.hpp"

static std::string intToString(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

Response UploadHandler::handle(const HttpRequest& req, const std::string &upload_dir)
{
    Response resp(req);
    //(void)upload_dir;
    if (!ensureDirectoryExists(upload_dir))
    {
        resp.setStatus(500);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
        return (resp);
    }
    std::vector<std::string> file_names = req.getFileNames();
    if (file_names.empty())
    {
        resp.setStatus(400);
        resp.addHeader("Content-Type", "text/html");
        resp.setBody("<html><body><h1>400 Bad Request</h1><p>No files uploaded</p></body></html>");
        return resp;
    }
    int success_count = 0;
    std::string result_html = "<html><body><h1>Upload Results</h1><ul>";
    for (size_t i = 0; i < file_names.size(); i++)
    {
        ParsedFile file = req.getFile(file_names[i]);
        if (saveFile(file, upload_dir))
        {
            result_html += "<li> <b>" + file.filename + "</b> uploaded (" 
            + intToString(file.size) + " bytes)</li>";
            success_count++;
        }
        else
            result_html += "<li><b>" + file.filename + "</b> failed</li>";

    }
    result_html += "</ul></body></html>";
    if (success_count == (int)file_names.size())
        resp.setStatus(201);
    else if (success_count > 0)
        resp.setStatus(207);
    else
        resp.setStatus(500);
    resp.addHeader("Content-Type", "text/html");
    resp.setBody(result_html);
    
    return resp;
}

bool UploadHandler::ensureDirectoryExists(const std::string &dir)
{
    struct stat st;
    if (stat(dir.c_str(), &st) == 0)
        return (S_ISDIR(st.st_mode));
    return (mkdir(dir.c_str(), 0755) == 0);
}

bool UploadHandler::saveFile(const ParsedFile& file, const std::string& dir)
{

    std::string filepath = dir + "/" + file.filename;

    std::ofstream out(filepath.c_str(), std::ios::binary);
    if (!out.is_open())
        return false;
    out.write(file.data.c_str(), file.size);
    out.close();
    return !out.fail();
}

    // bool saveTo(const std::string& directory) const
    // {
    //     if (filename.empty() || data.empty())
    //         return false;
    //     std::string filepath = directory;
    //     if (!filepath.empty() && filepath[filepath.length() - 1] != '/')
    //         filepath += "/";
    //     filepath += filename;
    //     //construit juste le chemin complet du file
    //     //dossier : uploads - file : test.txt = uploads/test.txt

    //     std::ofstream file(filepath.c_str(), std::ios::binary);
    //     //ouvrir le file en monde binaire
    //     //std::ios::binary est crucial pour file binary
    //     if (!file.is_open())
    //         return false;
    //     file.write(data.c_str(), data.size());
    //     //on utilise write pour du binaire pas << 
    //     file.close();
    //     return true;
    // }