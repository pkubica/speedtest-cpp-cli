#pragma once
#include <string>

//FOR COMPATIBILITY WITH C++0x

class Exception : public std::exception {
private:

public:
	std::string _message;
	Exception(const std::string &message)
	{
		_message = message;
	}
	virtual const char* what() const throw()
	{
		return _message.c_str();
	}
	~Exception() throw()
	{

	}
};
