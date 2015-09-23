#ifndef XMLFile_h__
#define XMLFile_h__

#include <string>
#include <fstream>
#include <stack>
#include <functional>

#include "xmlElement.h"

/*Terminology:
*<element>
*	<element attribute="value" attribute="value" />
*	<element attribute="value">content</element>
*</element>
*/
class XMLFile
{
public:
	explicit XMLFile();
	~XMLFile();

	bool Open(const std::string& path);
	void Close();

	bool ReadToNextElement();
	XMLElement ReadWholeElement();

	//************************************
	// Method:    	Parse
	// FullName:  	XMLFile::Parse
	// Access:    	public 
	// Returns:   	void
	// Qualifier: 	
	// Argument: 	std::function<void(XMLElement)> CallMethod - method to call whenever an element is parsed
	// Description: 
	//************************************
	void Parse(std::function<void(const XMLElement&)> CallMethod);
	XMLElement Parse();

	////////////////////////////////////////////////////////////
	//GETTERS
	////////////////////////////////////////////////////////////

	//************************************
	// Method:    	GetCurrentElement
	// FullName:  	XMLFile::GetCurrentElement
	// Access:    	public 
	// Returns:   	XMLElement
	// Qualifier: 	const
	// Description: Gets the last parsed element. NOTE: it may not be done parsing!
	//************************************
	XMLElement GetCurrentElement() const;

	std::string GetRootElementName() const;
private:
	//Amount of spaces a tab is equal to, change this if needed
	const int TAB_SPACES = 4;

	std::fstream stream;
	char currentChar;

	unsigned int currentIndex;
	unsigned int currentLine;

    std::string currentFile;

	XMLElement rootElement;
	std::stack<XMLElement*> elementStack;

	std::function<void(XMLElement)> CallMethod;

	////////////////////////////////////////////////////////////
	//PARSING
	////////////////////////////////////////////////////////////
	enum LAST_PARSE { NONE, HEADER, NEW_ELEMENT, CLOSE_ELEMENT, COMMENT };
	LAST_PARSE lastParse;

	void ReadToBeginBracket();
	//************************************
	// Method:    	ProcessBeginBracket
	// FullName:  	XMLFile::ProcessBeginBracket
	// Access:    	private 
	// Returns:   	void
	// Qualifier: 	
	// Description:	Does whatever needs to be done when a < is found
	//************************************
	void ProcessBeginBracket();

	//************************************
	// Method:    	ValidateEndTag
	// FullName:  	XMLFile::ValidateEndTag
	// Access:    	private 
	// Returns:   	bool - whether or not the tags matched
	// Qualifier: 	
	// Description: Checks whether or not the end tag's element matched the last begin tag's element
	//************************************
	bool ValidateEndTag();

	//************************************
	// Method:    	ParseElement
	// FullName:  	XMLFile::ParseElement
	// Access:    	private 
	// Returns:   	bool - whether or not to place this element on the stack
	// Qualifier: 	
	// Argument: 	XMLElement& outElement - element to put data into
	// Description: Parses element from the stream. This method assumes the beginning of an element has been found and currentChar/currentIndex are correct
	//************************************
	void ParseElement(XMLElement& outElement);
	//************************************
	// Method:    	ParseAttributes
	// FullName:  	XMLFile::ParseAttributes
	// Access:    	private 
	// Returns:   	bool - wheter or not to put this item in elementStack
	// Qualifier: 	
	// Argument: 	XMLElement& outElement - element to put data info
	// Description: Parses attributes from the current stream. This method assumes the end of an element has been found and currentChar/currentIndex are correct
	//************************************
	bool ParseAttributes(XMLElement& outElement);
	//************************************
	// Method:    	ParseContent
	// FullName:  	XMLFile::ParseContent
	// Access:    	private 
	// Returns:   	void
	// Qualifier: 	
	// Argument: 	XMLElement& outElement - element to put data into
	// Description: Parses content from the current stream. This method assumes the end of an element has been found and currentChar/currentIndex are correct
	//************************************
	void ParseContent(XMLElement& outElement);

	//************************************
	// Method:    	GetNextCharacter
	// FullName:  	XMLFile::GetNextCharacter
	// Access:    	private 
	// Returns:   	void
	// Qualifier: 	
	// Description: Gets the next character in the opened document and updates currentChar, currentIndex and currentLine. Ignores \n
	//************************************
	void GetNextCharacter(bool ignoreWhitespace = true);

	//************************************
	// Method:    	GetNextFromStream
	// FullName:  	XMLFile::GetNextFromStream
	// Access:    	private
	// Returns:   	void
	// Qualifier:
	// Description: IF YOU DO NOT KNOW WHAT THIS DOES THEN CALL GetNextCharacter!
	// 				Gets the next character from the input stream.
	//************************************
	void GetNextFromStream();

	void PutOnStack(const XMLElement& element);
	////////////////////////////////////////////////////////////
	//ERRORS
	////////////////////////////////////////////////////////////

	//************************************
	// Method:    	ShowErrorMessage
	// FullName:  	XMLFile::ShowErrorMessage
	// Access:    	private 
	// Returns:   	void
	// Qualifier: 	
	// Argument: 	const std::string & message
	// Description: Shows a MessageBox with the given message
	//************************************
	void ShowErrorMessage(const std::string& message);
	//************************************
	// Method:    	ShowErrorWithInfo
	// FullName:  	XMLFile::ShowErrorWithInfo
	// Access:    	private 
	// Returns:   	void
	// Qualifier: 	
	// Argument: 	const std::string & message
	// Description: Shows a MessageBox with the given message along with currentLine and currentIndex
	//************************************
	void ShowErrorWithInfo(const std::string& message);
	//************************************
	// Method:    	ShowUnexpectedCharacterErrorWithInfo
	// FullName:  	XMLFile::ShowUnexpectedCharacterErrorWithInfo
	// Access:    	private 
	// Returns:   	void
	// Qualifier: 	
	// Argument: 	const std::string & expected
	// Description: Shows a MessageBox with the following formatting: "Unexpected character! Expected <expected>, got currentLine[index]!" along with currentLine and currentIndex
	//************************************
	void ShowUnexpectedCharacterErrorWithInfo(const std::string& expected);
	//************************************
	// Method:    	ShowIllegalCharacterErrorWithInfo
	// FullName:  	XMLFile::ShowIllegalCharacterErrorWithInfo
	// Access:    	private 
	// Returns:   	void
	// Qualifier: 	
	// Description: Shows a MessageBox with the following formatting: "Illegal character found at line <currentLine>, index <index>!";
	//************************************
	void ShowIllegalCharacterErrorWithInfo();

	//************************************
	// Method:    	StreamError
	// FullName:  	XMLFile::StreamError
	// Access:    	private 
	// Returns:   	bool - !stream.is_open() && stream.eof()
	// Qualifier: 	
	// Description: Checks for a closed stream or eof
	//************************************
	bool StreamError();
};

#endif // XMLFile_h__
