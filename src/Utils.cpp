#include "Utils.hpp"
#include <sstream>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

std::string Utils::trim(const std::string& str) {
	size_t start = 0;
	size_t end = str.length();

	while (start < end && std::isspace(str[start]))
		start++;
	while (end > start && std::isspace(str[end - 1]))
		end--;

	return str.substr(start, end - start);
}

std::vector<std::string> Utils::split(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(str);

	while (std::getline(tokenStream, token, delimiter))
		tokens.push_back(token);

	return tokens;
}

std::string Utils::toLower(const std::string& str) {
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

std::string Utils::getMimeType(const std::string& path) {
	size_t pos = path.find_last_of('.');
	if (pos == std::string::npos)
		return "application/octet-stream";

	std::string ext = toLower(path.substr(pos + 1));

	if (ext == "html" || ext == "htm") return "text/html";
	if (ext == "css") return "text/css";
	if (ext == "js") return "application/javascript";
	if (ext == "json") return "application/json";
	if (ext == "png") return "image/png";
	if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
	if (ext == "gif") return "image/gif";
	if (ext == "svg") return "image/svg+xml";
	if (ext == "ico") return "image/x-icon";
	if (ext == "txt") return "text/plain";
	if (ext == "pdf") return "application/pdf";
	if (ext == "zip") return "application/zip";
	if (ext == "xml") return "application/xml";

	return "application/octet-stream";
}

std::string Utils::urlDecode(const std::string& str) {
	std::string result;
	char ch;
	int i, ii;

	for (i = 0; i < static_cast<int>(str.length()); i++) {
		if (str[i] == '%') {
			if (i + 2 < static_cast<int>(str.length())) {
				sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
				ch = static_cast<char>(ii);
				result += ch;
				i = i + 2;
			}
		} else if (str[i] == '+') {
			result += ' ';
		} else {
			result += str[i];
		}
	}

	return result;
}

bool Utils::fileExists(const std::string& path) {
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}

bool Utils::isDirectory(const std::string& path) {
	struct stat buffer;
	if (stat(path.c_str(), &buffer) != 0)
		return false;
	return S_ISDIR(buffer.st_mode);
}

std::string Utils::readFile(const std::string& path) {
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return "";

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

bool Utils::writeFile(const std::string& path, const std::string& content) {
	std::ofstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return false;

	file << content;
	file.close();
	return true;
}

bool Utils::deleteFile(const std::string& path) {
	return (unlink(path.c_str()) == 0);
}

std::string Utils::getDirectoryListing(const std::string& path, const std::string& uri) {
	DIR* dir = opendir(path.c_str());
	if (!dir)
		return "";

	std::string html = "<!DOCTYPE html>\n<html>\n<head>\n<title>Index of " + uri + "</title>\n";
	html += "<style>body{font-family:Arial;margin:20px;}h1{color:#333;}";
	html += "table{border-collapse:collapse;width:100%;}";
	html += "th,td{text-align:left;padding:8px;border-bottom:1px solid #ddd;}";
	html += "th{background-color:#4CAF50;color:white;}";
	html += "a{color:#1a73e8;text-decoration:none;}a:hover{text-decoration:underline;}</style>\n";
	html += "</head>\n<body>\n<h1>Index of " + uri + "</h1>\n<table>\n";
	html += "<tr><th>Name</th><th>Size</th></tr>\n";

	if (uri != "/")
		html += "<tr><td><a href=\"..\">..</a></td><td>-</td></tr>\n";

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;

		std::string fullPath = path + "/" + name;
		struct stat statbuf;

		if (stat(fullPath.c_str(), &statbuf) == 0) {
			std::string displayName = name;
			std::string size = "-";

			if (S_ISDIR(statbuf.st_mode)) {
				displayName += "/";
			} else {
				std::stringstream ss;
				ss << statbuf.st_size;
				size = ss.str();
			}

			html += "<tr><td><a href=\"" + uri + (uri[uri.length()-1] == '/' ? "" : "/") + name + "\">" + displayName + "</a></td><td>" + size + "</td></tr>\n";
		}
	}

	html += "</table>\n</body>\n</html>";
	closedir(dir);
	return html;
}

std::string Utils::intToString(int value) {
	std::stringstream ss;
	ss << value;
	return ss.str();
}

int Utils::stringToInt(const std::string& str) {
	std::stringstream ss(str);
	int value;
	ss >> value;
	return value;
}

size_t Utils::stringToSize(const std::string& str) {
	std::stringstream ss(str);
	size_t value;
	ss >> value;
	return value;
}
