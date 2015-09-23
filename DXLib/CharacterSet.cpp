#include "characterSet.h"

#include "constructedString.h"

#include "texture2D.h"
#include "characterBlock.h"

#include "logger.h"

CharacterSet::CharacterSet()
{
	this->name = "";
	fontSize = 0;
}

const Character* CharacterSet::GetCharacter(unsigned int id) const
{
	auto iter = characters.find(id);

	if(iter != characters.end())
		return &iter->second;
	else
	{
		iter = characters.find(errorCharacterID);

		if(iter != characters.end())
			return &iter->second;
		else
		{
			Logger::LogLine(LOG_TYPE::WARNING, "CharacterSet::errorCharacterID set to a non-existing character (make sure CharacterSet is loaded)");
			return &characters.begin()->second;
		}
	}
}

void CharacterSet::XMLSubscriber(const XMLElement& element)
{
	std::string elementName(element.GetName());

	if(elementName == "info")
	{
		fontSize = element.GetAttribute("size").GetValueAsUnsignedInt();
	}
	else if(elementName == "common")
	{
		lineHeight = element.GetAttribute("lineHeight").GetValueAsUnsignedInt();
	}
	else if(elementName == "char")
	{
		Character newChar(element.GetAttribute("id").GetValueAsUnsignedChar()
			, element.GetAttribute("x").GetValueAsShort()
			, element.GetAttribute("y").GetValueAsShort()
			, element.GetAttribute("width").GetValueAsChar()
			, element.GetAttribute("height").GetValueAsChar()
			, element.GetAttribute("xoffset").GetValueAsChar()
			, element.GetAttribute("yoffset").GetValueAsChar()
			, element.GetAttribute("xadvance").GetValueAsShort());

		characters.insert(std::pair<unsigned short, Character>(newChar.id, newChar));
	}
	else if(elementName == "kerning")
	{
        //unsigned char first = element.GetAttribute("first").GetValueAsUnsignedChar();
        //unsigned char second = element.GetAttribute("second").GetValueAsUnsignedChar();
        //char amount = element.GetAttribute("amount").GetValueAsChar();

		//characters[first].kerningPairs.insert(std::pair<unsigned char, char>(second, amount));
	}
}

std::string CharacterSet::GetName() const
{
	return name;
}

unsigned int CharacterSet::GetFontSize() const
{
	return fontSize;
}

ConstructedString CharacterSet::ConstructString(const std::string& text) const
{
	ConstructedString returnString;

	if(text == "")
	{
		std::vector<const Character*> newVector;
		returnString.characterBlocks.emplace_back(newVector, 0, 0);
		returnString.text = "";
		returnString.width = 0;
		returnString.length = 0;

		return returnString;
	}

	//Split each word (and the trailing blankspace) into a character block
	//If the string is something like "abc            def" split it into "abc" and " ... def" (ignoring a single space after abc)
	//A space is always presumed to be after a CharacterBlock when drawing
	bool splitAtSpace = false;

	unsigned int totalWidth = 0;

	CharacterBlock characterBlock;
	const Character* character;

	unsigned int length = 0;

	auto iter = text.begin();
	while(iter != text.end())
	{
		character = GetCharacter(static_cast<unsigned int>(*iter));
		++iter;

		if(character->id == SPACE_CHARACTER)
		{
			if(splitAtSpace)
			{
				splitAtSpace = false;

				returnString.characterBlocks.emplace_back(std::move(characterBlock));

				characterBlock.characters.clear();
				characterBlock.width = 0;
				characterBlock.length = 0;
				totalWidth += spaceXAdvance; //A space is always presumed to be after a character block so include it in the total width
				length++;
			}
			else
			{
				characterBlock.width += character->xAdvance;
				totalWidth += character->xAdvance;
				characterBlock.characters.emplace_back(character);
				characterBlock.length++;
				length++;
			}
		}
		else
		{
			splitAtSpace = true;

			characterBlock.width += character->xAdvance;
			totalWidth += character->xAdvance;
			characterBlock.characters.emplace_back(character);
			characterBlock.length++;
			length++;
		}
	}

	returnString.characterBlocks.emplace_back(std::move(characterBlock));

	returnString.width = totalWidth;
	returnString.text = text;
	returnString.length = length;

	return returnString;
}

ConstructedString CharacterSet::ConstructString(const std::string& text, const std::string& separators, const bool keepSeparators) const
{
	ConstructedString returnString;

	if(text == "")
	{
		std::vector<const Character*> newVector;
		returnString.characterBlocks.emplace_back(newVector, 0, 0);
		returnString.text = "";
		returnString.width = 0;
		returnString.length = 0;

		return returnString;
	}

	unsigned int totalWidth = 0;

	CharacterBlock characterBlock;
	const Character* character;

	unsigned int length = 0;

	auto iter = text.begin();
	while(iter != text.end())
	{
		character = GetCharacter(static_cast<unsigned int>(*iter));
		++iter;

		if(separators.find(static_cast<unsigned char>(character->id)) != separators.npos)
		{
			//Emplace current block
			returnString.characterBlocks.emplace_back(std::move(characterBlock));

			//Emplace block with separator if they should be kept
			if(keepSeparators)
			{
				characterBlock.width = static_cast<unsigned int>(character->xAdvance);
				totalWidth = static_cast<unsigned int>(character->xAdvance);
				characterBlock.characters.clear();
				characterBlock.characters.emplace_back(character);
				characterBlock.length = 1;
				length++;

				returnString.characterBlocks.emplace_back(std::move(characterBlock));
			}

			characterBlock.characters.clear();
			characterBlock.width = 0;
			characterBlock.length = 0;
			totalWidth += character->xAdvance;
			length++;
		}
		else
		{
			characterBlock.width += character->xAdvance;
			totalWidth += character->xAdvance;
			characterBlock.characters.emplace_back(character);
			characterBlock.length++;
			length++;
		}
	}

	returnString.characterBlocks.emplace_back(characterBlock);

	returnString.width = totalWidth;
	returnString.text = text;
	returnString.length = length;

	return returnString;
}

void CharacterSet::Insert(ConstructedString& constructedString, unsigned int index, const std::string& string) const
{
	//Turns out, just constructing a new string is ~1000 times faster than doing elaborate inserting/erasing.
	std::string text = constructedString.text;
	text.insert(index, string);

	constructedString = ConstructString(text);
}

void CharacterSet::Insert(ConstructedString& constructedString, int index, unsigned int character) const
{
	if(character > static_cast<unsigned int>(std::numeric_limits<char>::max()))
		character = '?';

	std::string text = constructedString.text;
	text.insert(static_cast<unsigned long>(index), 1, static_cast<char>(character));

	constructedString = ConstructString(text);
}

void CharacterSet::Erase(ConstructedString& constructedString, unsigned int startIndex, unsigned int count) const
{
	std::string text = constructedString.text;
	text.erase(startIndex, count);

	constructedString = ConstructString(text);
}

void CharacterSet::Replace(ConstructedString& constructedString, unsigned int begin, unsigned int end, const std::string& newText) const
{
	std::string text = constructedString.text;
	text.replace(begin, end, text);

	constructedString = ConstructString(text);
}

Texture2D* CharacterSet::GetTexture() const
{
	return texture;
}

bool CharacterSet::Load(const std::string& path, ID3D11Device* device, ContentManager* contentManager /*= nullptr*/, ContentParameters* contentParameters /*= nullptr*/)
{
	characters.clear();

	this->name = path;

	XMLFile xmlFile;
	xmlFile.Open(path + ".fnt");
	xmlFile.Parse(std::bind(&CharacterSet::XMLSubscriber, this, std::placeholders::_1));

	texture = contentManager->Load<Texture2D>(path + ".dds");

	if(characters.size() > 0)
	{
		characters.insert(std::pair<unsigned short, Character>('\n', Character('\n', 0, 0, 0, fontSize + lineHeight, 0, 0, 0)));

		//If the user expects to use spaces it is expected to be in the .xml-file
		spaceXAdvance = static_cast<unsigned int>(GetCharacter(SPACE_CHARACTER)->xAdvance);
	}

	return true;
}

void CharacterSet::Unload(ContentManager* contentManager)
{
	if(texture != nullptr)
		contentManager->Unload(texture);
}

unsigned int CharacterSet::GetLineHeight() const
{
	return lineHeight;
}

unsigned int CharacterSet::GetWidthAtMaxWidth(const ConstructedString& string, unsigned int maxWidth) const
{
	unsigned int currentWidth = 0;

	for(const CharacterBlock& block : string.characterBlocks)
	{
		if(currentWidth + block.width > maxWidth)
		{
			short characterXAdvance;

			for(const Character* character : block.characters)
			{
				characterXAdvance = character->xAdvance;

				currentWidth += characterXAdvance;

				if(currentWidth >= maxWidth)
					return currentWidth - characterXAdvance;
			}
		}
		else
			currentWidth += block.width;
	}

	return currentWidth;
}

unsigned int CharacterSet::GetWidthAtIndex(const ConstructedString& constructedString, unsigned int index) const
{
	if(index >= constructedString.text.size())
		return constructedString.width;

	unsigned int currentWidth = 0;
	unsigned int currentIndex = 0;

	for(const CharacterBlock& block : constructedString.characterBlocks)
	{
		unsigned int blockSize = static_cast<unsigned int>(block.characters.size());

		//Is the index inside the current block?
		if(currentIndex + blockSize > index)
		{
			for(unsigned int i = 0; i < index - currentIndex; i++)
				currentWidth += block.characters[i]->xAdvance;

			return currentWidth;
		}
		else if(currentIndex + blockSize == index)
		{
			return currentWidth + block.width;
		}
		else
		{
			currentWidth += block.width;
			currentIndex += blockSize;
		}

		currentWidth += spaceXAdvance;
		currentIndex++;

		if(currentIndex == index)
			return currentWidth;
	}

	return currentWidth;
}

unsigned int CharacterSet::GetWidthAtIndex(const std::string& text, unsigned index) const
{
	if(index == 0)
		return 0;

	unsigned int currentWidth = 0;
	unsigned int currentIndex = 0;

	for(char stringCharacter : text)
	{
		const Character* character = GetCharacter(stringCharacter);

		currentWidth += character->xAdvance;
		++currentIndex;

		if(currentIndex == index)
			return currentWidth;
	}

	return currentWidth;
}

unsigned int CharacterSet::GetIndexAtWidth(const ConstructedString& constructedString, unsigned int width, float roundingValue /*= 0.6f*/) const
{
	if(width >= static_cast<int>(constructedString.width))
		return constructedString.length;

	unsigned int currentWidth = 0;
	unsigned int currentIndex = 0;

	//Used for index rounding
	int lastCharXAdvance = 0;

	for(const CharacterBlock& characterBlock : constructedString.characterBlocks)
	{
		if(currentWidth + static_cast<unsigned int>(characterBlock.width) < width)
		{
			currentWidth += characterBlock.width;
			currentIndex += static_cast<unsigned int>(characterBlock.characters.size());
		}
		else
		{
			for(const Character* character : characterBlock.characters)
			{
				if(currentWidth + character->xAdvance <= width)
				{
					currentWidth += character->xAdvance;
					currentIndex++;
				}
				else
				{
					lastCharXAdvance = character->xAdvance;
					break;
				}
			}

			break;
		}

		//Space
		if(currentWidth + static_cast<int>(spaceXAdvance) <= width)
		{
			currentWidth += spaceXAdvance;
			currentIndex++;
		}
		else
		{
			lastCharXAdvance = spaceXAdvance;
			break;
		}
	}

	currentIndex += (currentWidth + lastCharXAdvance - width) < (lastCharXAdvance * roundingValue);

	return currentIndex;
}

unsigned int CharacterSet::GetRowsAtWidth(const ConstructedString& constructedString, unsigned int width) const
{
	unsigned int rows = 1;
	unsigned int currentWidth = 0;

	for(const CharacterBlock& block : constructedString.characterBlocks)
	{
		if(currentWidth + block.width <= width)
		{
			currentWidth += block.width + GetSpaceXAdvance();
		}
		else
		{
			if(block.width > width)
			{
				//Block needs to be split into several lines

				for(const Character* character : block.characters)
				{
					if(currentWidth + character->xAdvance < width)
						currentWidth += character->xAdvance;
					else
					{
						currentWidth = static_cast<unsigned int>(character->xAdvance);
						rows++;
					}
				}

				currentWidth += GetSpaceXAdvance();
			}
			else
			{
				currentWidth = block.width + GetSpaceXAdvance();
				rows++;
			}
		}
	}

	return rows;
}

int CharacterSet::GetSpaceXAdvance() const
{
	return spaceXAdvance;
}