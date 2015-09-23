#include "xmlFile.h"

#include "Logger.h"

XMLFile::XMLFile()
	: stream()
	, currentChar(0)
	, currentIndex(0)
	, currentLine(0)
	, lastParse(NONE)
{
}

XMLFile::~XMLFile()
{
}

bool XMLFile::Open(const std::string& path)
{
	if(stream.is_open())
		Close();

	stream.open(path, std::ios::in);

	if(StreamError())
	{
		ShowErrorMessage("Couldn't open file at " + path);
		return false;
	}

    currentFile = path;

	return true;
}

void XMLFile::Close()
{
	if(stream.is_open())
		stream.close();

	while(elementStack.size() > 0)
		elementStack.pop();

	currentChar = 0;
	currentIndex = 0;
	currentLine = 0;
    currentFile = "";
}

bool XMLFile::ReadToNextElement()
{
	if(!StreamError())
	{
		lastParse = NONE;

		//Read until a new element has been parsed
		while(lastParse != NEW_ELEMENT)
		{
			ReadToBeginBracket();
			if(StreamError())
				return true;

			ProcessBeginBracket();
			if(StreamError())
				return true;
		}

		return false;
	}

	return true;
}

XMLElement XMLFile::ReadWholeElement()
{
	if(!StreamError() && elementStack.size() > 0)
	{
		std::string currentElement = elementStack.top()->name;
		unsigned int currentSize = static_cast<unsigned int>(elementStack.size());

		lastParse = NONE;

		//When this is true the entire element has been parsed
		while(elementStack.size() != currentSize - 1)
		{
			ReadToBeginBracket();
			if(StreamError())
				return XMLElement();

			ProcessBeginBracket();
			if(StreamError())
				return XMLElement();
		}

		unsigned int size = static_cast<unsigned int>(elementStack.top()->children[currentElement].size());

		return elementStack.top()->children[currentElement][size - 1];
	}

	return XMLElement();
}

void XMLFile::Parse(std::function<void(const XMLElement&)> CallMethod)
{
	if(!StreamError())
	{
		this->CallMethod = CallMethod;

		//eof will be reached eventually
		while(true)
		{
			ReadToBeginBracket();
			if(StreamError())
				break;

			ProcessBeginBracket();
			if(StreamError())
				break;
		}

		this->CallMethod = nullptr;
	}

	Close();
}

XMLElement XMLFile::Parse()
{
	if(!StreamError())
	{
		//eof will be reached eventually
		while(true)
		{
			ReadToBeginBracket();
			if(StreamError())
				break;

			ProcessBeginBracket();
			if(StreamError())
				break;
		}

		return rootElement;
	}

	Close();
	return XMLElement();
}

////////////////////////////////////////////////////////////
//PARSING
////////////////////////////////////////////////////////////
void XMLFile::ReadToBeginBracket()
{
	//Either the next element will be found or an error will occur
	while(true)
	{
		while(currentChar == ' ' || currentChar == '\t' || currentChar == 0)
			GetNextCharacter();

		if(currentChar == '<')
			break;
		else if(currentChar != ' ' && currentChar != '\t')
		{
			ShowUnexpectedCharacterErrorWithInfo("<");

			Close();
			break;
		}
	}
}

void XMLFile::ProcessBeginBracket()
{
	GetNextCharacter();

	/*There are three cases:
	* <? (ignored)
	* <! (ignore until "-->")
	* <tag ... (parse element)
	* </tag (validate)
	*/
	if(currentChar == '?')
	{
		lastParse = HEADER;

		//Either eof will be reached (error) or an > will be found (continue)
		while(true)
		{
			GetNextCharacter();


			//If eof is reached the file is invalid
			if(StreamError())
			{
				ShowUnexpectedCharacterErrorWithInfo(">");

				Close();
				break;
			}
			else if(currentChar == '>') //Found end of header
				break;
		}
	}
	else if(currentChar == '!') //Comment
	{
		lastParse = COMMENT;

		char next[3] { '-', '-', '>' };
		int count = 0;

		//Make sure the next to characters are --
		while(count < 2)
		{
			GetNextCharacter();

			if(currentChar == next[count])
				count++;
			else
			{
				std::string message = "";
				message += next[count];
				ShowUnexpectedCharacterErrorWithInfo(message);

				Close();
				return;
			}
		}

		count = 0;
		//Ignore everything until --> is found
		while(count < 3)
		{
			GetNextCharacter();

			if(currentChar == next[count])
				count++;
			else
				count = 0;
		}
	}
	else if(currentChar == '/') //If currenChar is / the end tag of an element was found
	{
		lastParse = CLOSE_ELEMENT;

		if(ValidateEndTag())
			elementStack.pop();
	}
	else //The next char wasn't a / so this is a new element
	{
		if(!isalpha(currentChar) && !isdigit(currentChar))
		{
			ShowIllegalCharacterErrorWithInfo();

			Close();
			return;
		}

		lastParse = NEW_ELEMENT;

		XMLElement newElement;

		ParseElement(newElement);
		if(StreamError())
			return;

		bool putOnStack = ParseAttributes(newElement);
		if(StreamError())
			return;

		ParseContent(newElement);
		if(StreamError())
			return;

		//If the element should't be put on the stack just add it as a child to the current top
		if(putOnStack)
			PutOnStack(newElement);
		else
			elementStack.top()->children[newElement.name].emplace_back(newElement);

		if(CallMethod != nullptr)
			CallMethod(newElement);
	}

	if(currentChar == '>')
		GetNextCharacter(); //Skip >
}

bool XMLFile::ValidateEndTag()
{
	//Make sure the end tag matches the begin
	std::string closedElement = "";

	//Get the tag after / and before > (if it exists, otherwise error)
	bool endFound = false;
	while(!stream.eof())
	{
		GetNextCharacter();

		if(currentChar == '>')
		{
			endFound = true;
			break;
		}
		else if(currentChar == ' ')
		{
			ShowErrorWithInfo("Space found where end bracket was expected!");

			Close();
			return false;
		}

		closedElement += currentChar;
	}

	//Didn't find a > on this line (error)
	if(!endFound)
	{
		ShowErrorWithInfo("No end bracket found where one was expected!");

		Close();
		return false;
	}

	//This should absolutely not happen!
	if(elementStack.size() == 0)
	{
		ShowErrorWithInfo("Element stack is empty! Find David!");

		Close();
		return false;
	}

	//Make sure the open tag and close tag match, otherwise error
	if(closedElement != elementStack.top()->GetName())
	{
		std::string top = elementStack.top()->GetName();
		ShowErrorWithInfo("Close doesn't match open! Expected \"" + std::string(top.begin(), top.end()) + "\", got \"" + std::string(closedElement.begin(), closedElement.end()) + "\"!");

		Close();
		return false;
	}
	else
		return true;
}

void XMLFile::ParseElement(XMLElement& outElement)
{
	std::string newElementName = "";

	bool elementFound = false;

	//Append chars until < or space is found, but allow only numbers and letters
	while(!stream.eof())
	{
		if(currentChar == ' ' || currentChar == '>')
		{
			elementFound = true;
			break;
		}
		else if(!isalpha(currentChar) && !isdigit(currentChar))
		{
			ShowUnexpectedCharacterErrorWithInfo("letter or number");

			Close();
			outElement = XMLElement();
			return;
		}

		newElementName += currentChar;

		GetNextCharacter(false); //At the end since currentChar will be first letter of elementName when this method is called
	}

	if(!elementFound)
	{
		ShowErrorWithInfo("New element expected but none found!");

		Close();
		outElement = XMLElement();
	}

	outElement.name = newElementName;
}

bool XMLFile::ParseAttributes(XMLElement& outElement)
{
	//This should always return true as long as no "/" is found (or an error occurs)

	//If there are attributes there will be text, not a closing bracket
	if(currentChar == '>')
		return true;

	enum CurrentParse { NAME, ASSIGN, VALUE };

	CurrentParse currentParse = NAME;

	std::string name = "";
	std::string value = "";
	while(!StreamError())
	{
		switch(currentParse)
		{
			case NAME:
				/*There are 6 cases:
				* currentChar is > (new element, return)
				* currentChar is '/' (single-line element, return)
				* currentChar is ' ' and name is empty (continue)
				* currentChar is '=' (append currentLine[i] to name and change currentParse to ASSIGN and continue)
				* currentChar isn't a letter, a number or a '=' (error)
				* None of the above are true (append currentLine[i] to name)
				*/
				switch(currentChar)
				{
					case '>':
						return true;
					case '/':
						GetNextCharacter();
						return false;
					case ' ':
						if(!name.empty())
						{
							ShowUnexpectedCharacterErrorWithInfo("=");

							Close();
							return true;
						}
						break;
					case '=':
						currentParse = ASSIGN;
						break;
					default:
						name += currentChar;
						break;
				}
				break;
			case ASSIGN:
				if(currentChar != '\"')
				{
					ShowUnexpectedCharacterErrorWithInfo("\"");

					Close();
					return true;
				}

				currentParse = VALUE;
				break;
			case VALUE:
				/*There are 3 cases:
				* currentChar is an " (append currentLine[i] to value, emplace a new attribute, reset and then change currentParse to NAME and continue)
				* currentChar isn't a letter, a number or a " (error)
				* None of the above are true (append currentLine[i] to value)
				*/
				while(true)
				{
					if(currentChar == '\"')
					{
						outElement.attributes.insert(std::pair<std::string, XMLAttribute>(name, XMLAttribute(name, value)));

						name = "";
						value = "";

						currentParse = NAME;
						break;
					}
					else
						value += currentChar;

					GetNextCharacter(false);

					if(StreamError())
					{
						ShowErrorMessage("Ran out of file to read while parsing attributes for \"" + outElement.name + "\". Did you forget a \"? The parsed attribute was \"" + name + "\"");

						Close();
						return true;
					}
				}
				break;
			default:
				ShowErrorWithInfo("Hit default in ParseAttributes! (Yell at David (or, more likely, the dumbass who modfied an enum without searching for usages))");
				break;
		}

		GetNextCharacter(); //At the end since currentChar will be first letter or the attribute when this method is called
	}

	ShowErrorWithInfo("Unexpected error! Check syntax!");

	Close();
	return false;
}

void XMLFile::ParseContent(XMLElement& outElement)
{
	GetNextCharacter();

	while(currentChar == ' ' || currentChar == '\t')
		GetNextCharacter();

	if(currentChar == '<') //No content
		return;

	std::string content = "";

	while(currentChar != '<')
	{
		content += currentChar;

		if(StreamError())
		{
			ShowErrorWithInfo("Unexpected end of file reached!");

			Close();
			return;
		}

		GetNextCharacter();
	}

	//If we're out there it must mean currentChar == <
	outElement.contents = content;
}

void XMLFile::GetNextCharacter(bool ignoreWhitespace /* = true*/)
{
	if(!StreamError())
	{
		if(ignoreWhitespace)
		{
			do
			{
				GetNextFromStream();
			} while(isspace(currentChar));
		}
		else
			GetNextFromStream();
	}
}

void XMLFile::GetNextFromStream()
{
	currentChar = static_cast<char>(stream.get());

	switch(currentChar)
	{
		case '\t':
			currentIndex += TAB_SPACES;
			break;
		case '\r':
			break;
		case '\n':
			while(currentChar == '\n')
				currentChar = static_cast<char>(stream.get());

			currentLine++;
			currentIndex = 0;
			break;
		default:
			currentIndex++;
			break;
	}
}

void XMLFile::PutOnStack(const XMLElement& element)
{
	//If stack is empty -> store rootElement
	if(elementStack.size() == 0)
	{
		rootElement = element;

		elementStack.emplace(&rootElement);
	}
	else
	{
		elementStack.top()->children[element.name].emplace_back(element);

		elementStack.emplace(&elementStack.top()->children[element.name].back());
	}
}

////////////////////////////////////////////////////////////
//ERRORS
////////////////////////////////////////////////////////////
void XMLFile::ShowErrorMessage(const std::string& message)
{
    Logger::LogLine(LOG_TYPE::FATAL, "XML-error while parsing \"" + currentFile + "\": " + message);
}

void XMLFile::ShowUnexpectedCharacterErrorWithInfo(const std::string& expected)
{
	//Unexpected character! Expected <expected>, got currentLine[index]!
    std::string fullMessage = "XML-error while parsing \"" + currentFile + "\": " + "Unexpected character! Expected " + expected + ", got '" + currentChar + "'!";

	//0-indexed so add 1
	fullMessage += " Line number " + std::to_string(currentLine + 1)
				+ ", index " + std::to_string(currentIndex + 1);

	Logger::LogLine(LOG_TYPE::FATAL, fullMessage);
}

void XMLFile::ShowErrorWithInfo(const std::string& message)
{
    std::string fullMessage = "XML-error while parsing \"" + currentFile + "\": " + message;

	//0-indexed so add 1
	fullMessage += " Line number " + std::to_string(currentLine + 1)
				+ ", index " + std::to_string(currentIndex + 1);

	Logger::LogLine(LOG_TYPE::FATAL, fullMessage);
}

void XMLFile::ShowIllegalCharacterErrorWithInfo()
{
    std::string fullMessage = "XML-error while parsing \"" + currentFile + "\": " + "Illegal character found at";

	//0-indexed so add 1
	fullMessage += " line " + std::to_string(currentLine + 1)
		+ ", index " + std::to_string(currentIndex + 1) + "!";

	Logger::LogLine(LOG_TYPE::FATAL, fullMessage);
}

bool XMLFile::StreamError()
{
	return !stream.is_open() || stream.eof();
}

////////////////////////////////////////////////////////////
//GETTERS
////////////////////////////////////////////////////////////
XMLElement XMLFile::GetCurrentElement() const
{
	if(elementStack.size() > 0)
		return *elementStack.top();
	else
		return XMLElement();
}

std::string XMLFile::GetRootElementName() const
{
	return rootElement.name;
}
