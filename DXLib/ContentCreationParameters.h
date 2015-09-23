#ifndef ContentCreationParameters_h__
#define ContentCreationParameters_h__

#include "contentParameters.h"

#include <string>

struct ContentCreationParameters :
	public ContentParameters
{
	ContentCreationParameters()
		:uniqueID("")
	{}
	virtual ~ContentCreationParameters() {}

	std::string uniqueID;
};

#endif // ContentCreationParameters_h__
