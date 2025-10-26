#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

class Utils {
public:
	static std::string trim(const std::string& str);
	static std::vector<std::string> split(const std::string& str, char delimiter);
	static std::string toLower(const std::string& str);
	static std::string getMimeType(const std::string& path);
	static std::string urlDecode(const std::string& str);
	static bool fileExists(const std::string& path);
	static bool isDirectory(const std::string& path);
	static std::string readFile(const std::string& path);
	static bool writeFile(const std::string& path, const std::string& content);
	static bool deleteFile(const std::string& path);
	static std::string getDirectoryListing(const std::string& path, const std::string& uri);
	static std::string intToString(int value);
	static int stringToInt(const std::string& str);
	static size_t stringToSize(const std::string& str);
};

#endif
