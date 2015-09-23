#ifndef Character_h__
#define Character_h__

class Character
{
public:
	Character(void)
		: id(0)
		, x(0)
		, y(0)
		, width(0)
		, height(0)
		, xOffset(0)
		, yOffset(0)
		, xAdvance(0) {}
	Character(int id, unsigned short x, unsigned short y, unsigned char width, unsigned char height, char xOffset, char yOffset, unsigned short xAdvance)
		: id(id)
		, x(x)
		, y(y)
		, width(width)
		, height(height)
		, xOffset(xOffset)
		, yOffset(yOffset)
		, xAdvance(xAdvance) {}
	~Character(void) {};

	int id;

	unsigned short x;
	unsigned short y;

	unsigned char width;
	unsigned char height;

	char xOffset;
	char yOffset;

	short xAdvance;

	//std::unordered_map<unsigned char, char> kerningPairs; //TODO: Does this have to be here?
};

#endif // Character_h__
