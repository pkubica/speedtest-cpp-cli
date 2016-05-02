#include <string>
#include <unordered_map>
#include "TAGParser.hpp"


size_t TAGParser::parseTagAttributes(std::string &data, std::unordered_map<std::string, std::string> &attributes, size_t pos)
{
	int state = 0;
	std::string name = "";
	std::string value = "";
	for (; pos < data.size(); ++pos)
	{
		if (state == 0 && data[pos] == '/')
		{
			//cout << "returning" << endl;
			return pos;
		}

		switch (state)
		{
		case 0:
			if (data[pos] == '=')
			{
				++state;
				continue;
			}
			name += data[pos];
			continue;
		case 1:
			if (data[pos] == '"')
			{
				++state;
				continue;
			}
			else
			{
				//TODO: parse error, missing started "
			}
			continue;
		case 2:
			if (data[pos] == '"')
			{
				attributes.insert(std::pair<std::string, std::string>(name, value));
				name = "";
				value = "";
				++state;
				continue;
			}
			value += data[pos];
			continue;
		case 3:
			//cout << "name:" << name << " value:" << value << endl;
			state = 0;
			continue;
		}
	}
	return pos;
}

std::string TAGParser::getTagByName(std::string name, std::string &data)
{
	size_t pos = data.find("<" + name + " ");
	if (pos == std::string::npos)
	{
		return "";
	}
	pos += name.size() + 2;

	size_t end_pos = data.find(">", pos);
	if (end_pos == std::string::npos)
	{
		return "";
	}

	if (data.at(end_pos - 1) == '/')
	{
		end_pos -= 1;
	}

	while (data.at(end_pos - 1) == ' ')
	{
		end_pos -= 1;
	}

	return data.substr(pos, end_pos - pos);
}
