#ifndef Texture2DCreateParameters_h__
#define Texture2DCreateParameters_h__

#include "contentCreationParameters.h"
#include "texture2DParameters.h"

struct Texture2DCreateParameters :
	public ContentCreationParameters
{
	Texture2DCreateParameters()
		: data(nullptr)
		, width(-1)
		, height(-1)
	{

	}

	~Texture2DCreateParameters() {};

	Texture2DParameters texParams;

	uint8_t* data;

	int width;
	int height;
};

#endif // Texture2DCreateParameters_h__
