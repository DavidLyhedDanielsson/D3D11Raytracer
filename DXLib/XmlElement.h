#ifndef XMLElement_h__
#define XMLElement_h__

#include <string>
#include <vector>
#include <map>

#include "xmlAttribute.h"

class XMLElement
{
	friend class XMLFile;
public:
	explicit XMLElement();
	explicit XMLElement(const std::string& name);
	explicit XMLElement(const std::string& name, const std::map<std::string, XMLAttribute>& attributes);
	explicit XMLElement(const std::string& name, const std::map<std::string, XMLAttribute>& attributes, const std::string& contents);
	~XMLElement();

	std::string	GetName() const;
	std::string	GetContents() const;
	XMLAttribute GetAttribute(const std::string& name) const;
	bool AttributeExists(const std::string& name) const;
	std::vector<XMLElement>	GetChildren(const std::string& name) const;
	bool ChildExists(const std::string& name) const;

	std::string	ToFormattedString() const;
private:
	std::string name;
	std::string contents;

	std::map<std::string, XMLAttribute> attributes;
	std::map<std::string, std::vector<XMLElement>> children;

	std::string ToFormattedStringRec(int indent) const;
};

inline bool operator==(const XMLElement& element, const std::string& name)
{
	return element.GetName() == name;
}

inline bool operator==(const std::string& name, const XMLElement& element)
{
	return operator==(element, name);
}

#endif // XMLElement_h__
