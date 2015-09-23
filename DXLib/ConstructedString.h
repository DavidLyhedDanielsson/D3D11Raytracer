#ifndef ConstructedString_h__
#define ConstructedString_h__

#include <vector>
#include <string>

#include "characterBlock.h"

struct ConstructedString
{
	ConstructedString() 
		: width(0)
		, text("")
		, length(0)
	{};
	ConstructedString(std::vector<CharacterBlock> characterBlocks, unsigned int width, std::string text, unsigned int length)
		: characterBlocks(characterBlocks)
		, width(width)
		, text(text)
		, length(length) {};
	~ConstructedString() = default;

	std::vector<CharacterBlock> characterBlocks;
	unsigned int width;
	std::string text;
	unsigned int length;
};

inline bool operator==(const ConstructedString& lhs, const std::string& rhs)
{
	return lhs.text == rhs;
}

inline bool operator!=(const ConstructedString& lhs, const std::string& rhs)
{
	return !(lhs == rhs);
}

inline bool operator==(const std::string& lhs, const ConstructedString& rhs)
{
	return rhs == lhs;
}

inline bool operator!=(const std::string& lhs, const ConstructedString& rhs)
{
	return !(rhs == lhs);
}

#endif // ConstructedString_h__
