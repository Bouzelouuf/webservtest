#include "LocationConfig.hpp"

LocationConfig::LocationConfig() : autoindex(false) {}

LocationConfig::LocationConfig(const std::string& p) : path(p), autoindex(false) {}

LocationConfig::~LocationConfig() {}

void LocationConfig::setPath(const std::string& p) {
	path = p;
}

void LocationConfig::setRoot(const std::string& r) {
	root = r;
}

void LocationConfig::setIndex(const std::string& i) {
	index = i;
}

void LocationConfig::addAllowedMethod(const std::string& method) {
	allowedMethods.insert(method);
}

void LocationConfig::setAutoindex(bool value) {
	autoindex = value;
}

void LocationConfig::setUploadPath(const std::string& path) {
	uploadPath = path;
}

void LocationConfig::setCgiExtension(const std::string& ext) {
	cgiExtension = ext;
}

void LocationConfig::setCgiPath(const std::string& path) {
	cgiPath = path;
}

void LocationConfig::setRedirect(const std::string& redir) {
	redirect = redir;
}

const std::string& LocationConfig::getPath() const {
	return path;
}

const std::string& LocationConfig::getRoot() const {
	return root;
}

const std::string& LocationConfig::getIndex() const {
	return index;
}

const std::set<std::string>& LocationConfig::getAllowedMethods() const {
	return allowedMethods;
}

bool LocationConfig::getAutoindex() const {
	return autoindex;
}

const std::string& LocationConfig::getUploadPath() const {
	return uploadPath;
}

const std::string& LocationConfig::getCgiExtension() const {
	return cgiExtension;
}

const std::string& LocationConfig::getCgiPath() const {
	return cgiPath;
}

const std::string& LocationConfig::getRedirect() const {
	return redirect;
}

bool LocationConfig::isMethodAllowed(const std::string& method) const {
	if (allowedMethods.empty())
		return true;
	return allowedMethods.find(method) != allowedMethods.end();
}
