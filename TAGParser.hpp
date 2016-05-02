#pragma once

#include <string>
#include <unordered_map>

class TAGParser
{
public:
	static size_t parseTagAttributes(std::string &data, std::unordered_map<std::string, std::string> &attributes, size_t pos = 0);
	static std::string getTagByName(std::string name, std::string &data);

};