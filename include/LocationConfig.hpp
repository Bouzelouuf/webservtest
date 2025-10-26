#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>
#include <set>

class LocationConfig {
private:
	std::string path;
	std::string root;
	std::string index;
	std::set<std::string> allowedMethods;
	bool autoindex;
	std::string uploadPath;
	std::string cgiExtension;
	std::string cgiPath;
	std::string redirect;

public:
	LocationConfig();
	LocationConfig(const std::string& p);
	~LocationConfig();

	void setPath(const std::string& p);
	void setRoot(const std::string& r);
	void setIndex(const std::string& i);
	void addAllowedMethod(const std::string& method);
	void setAutoindex(bool value);
	void setUploadPath(const std::string& path);
	void setCgiExtension(const std::string& ext);
	void setCgiPath(const std::string& path);
	void setRedirect(const std::string& redir);

	const std::string& getPath() const;
	const std::string& getRoot() const;
	const std::string& getIndex() const;
	const std::set<std::string>& getAllowedMethods() const;
	bool getAutoindex() const;
	const std::string& getUploadPath() const;
	const std::string& getCgiExtension() const;
	const std::string& getCgiPath() const;
	const std::string& getRedirect() const;
	bool isMethodAllowed(const std::string& method) const;
};

#endif
