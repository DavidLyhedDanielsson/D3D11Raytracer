#ifndef CharacterSet_h__
#define CharacterSet_h__

#include "character.h"

#include <unordered_map>

#include "constructedString.h"
#include "texture2D.h"
#include "content.h"
#include "xmlFile.h"
#include "contentManager.h"

class CharacterSet : public Content
{
public:
	CharacterSet();
	~CharacterSet() = default;

	//It's probably a terrible idea to copy this class
	CharacterSet& operator=(const CharacterSet& rhs) = delete;
	CharacterSet& operator=(CharacterSet&& rhs) = delete;

	const static int SPACE_CHARACTER = ' '; //Change this if needed. Should correspond to your desired value for a blankspace

	std::string GetName() const;
	unsigned int GetFontSize() const;
	Texture2D* GetTexture() const;

	int GetSpaceXAdvance() const;

	const Character* GetCharacter(unsigned int id) const;

	ConstructedString ConstructString(const std::string& text) const;
	ConstructedString ConstructString(const std::string& text, const std::string& separators, const bool keepSeparators) const;
	//************************************
	// Method:		InsertCharacter
	// FullName:	CharacterSet::InsertCharacter
	// Access:		public 
	// Returns:		void
	// Qualifier:	const
	// Argument:	ConstructedString& constructedString
	// Argument:	int index
	// Argument:	unsigned int character
	// Description:	Inserts character at the index into constructedString
	//************************************
    void Insert(ConstructedString& constructedString, unsigned int index, const std::string& string) const;
	void Insert(ConstructedString& constructedString, int index, unsigned int character) const;

	void Erase(ConstructedString& constructedString, unsigned int startIndex, unsigned int count) const;

	//Replaces a character block
	void Replace(ConstructedString& constructedString, unsigned int begin, unsigned int end, const std::string& newText) const;

	//************************************
	// Method:		GetLineHeight
	// FullName:	CharacterSet::GetLineHeight
	// Access:		public 
	// Returns:		unsigned int
	// Qualifier:	const
	// Description:	Returns the line height for the currently loaded font
	//************************************
	unsigned int GetLineHeight() const;
	//************************************
	// Method:		GetWidthAtMaxWidth
	// FullName:	CharacterSet::GetWidthAtMaxWidth
	// Access:		public 
	// Returns:		unsigned int - width
	// Qualifier:	const
	// Argument:	const ConstructedString& constructedString
	// Argument:	unsigned int maxWidth - width to measure to
	// Description:	Returns the (graphical) width of constructedString by counting characters until maxWidth is reached or would be exceeded at the next character
	//************************************
	unsigned int GetWidthAtMaxWidth(const ConstructedString& constructedString, unsigned int maxWidth) const;
	//************************************
	// Method:		GetWidthAtIndex
	// FullName:	CharacterSet::GetWidthAtIndex
	// Access:		public 
	// Returns:		unsigned int - width
	// Qualifier:	const
	// Argument:	const ConstructedString& constructedString
	// Argument:	unsigned int index - character index to measure to
	// Description:	Returns the (graphical) width of constructedString from the start to index
	//************************************
	unsigned int GetWidthAtIndex(const ConstructedString& constructedString, unsigned int index) const;
	unsigned int GetWidthAtIndex(const std::string& text, unsigned int index) const;

	//************************************
	// Method:		GetIndexAtWidth
	// FullName:	CharacterSet::GetIndexAtWidth
	// Access:		public 
	// Returns:		unsigned int - index
	// Qualifier:	const
	// Argument:	const ConstructedString& constructedString
	// Argument:	int width - width to measure to
	// Argument:	float roundingValue - index rouning value. If this value is 0.5 and the user clicks in the middle of the character the method will return the index after the character. If this value is 0.55 and the user clicks in the middle the method will return the index of the current character
	// Description:	
	//************************************
	unsigned int GetIndexAtWidth(const ConstructedString& constructedString, unsigned int width, float roundingValue = 0.6f) const;

	//************************************
	// Method:		GetRowsAtWidth
	// Returns:		unsigned int
	// Argument:	const ConstructedString& constructedString
	// Argument:	int width - width to measure to
	// Description:	Returns the amount of rows the given string will have at the given width
	//************************************
	unsigned int GetRowsAtWidth(const ConstructedString& constructedString, unsigned int width) const;
private:
    const unsigned int errorCharacterID = 0x3F; //0x3F = "?"

	void XMLSubscriber(const XMLElement& element);
	//char GetKerningOffset(char first, char second) const; //TODO: Save kerning pairs in this class

    bool Load(const std::string& path, ID3D11Device* device, ContentManager* contentManager = nullptr, ContentParameters* contentParameters = nullptr) override;
    void Unload(ContentManager* contentManager = nullptr) override;

	std::string name;
	std::unordered_map<unsigned int, Character> characters;

	unsigned int fontSize;
	unsigned int lineHeight;
	unsigned int spaceXAdvance;

	Texture2D* texture;
};

#endif // CharacterSet_h__
