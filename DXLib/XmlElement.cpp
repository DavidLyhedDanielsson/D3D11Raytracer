#include "xmlElement.h"

XMLElement::XMLElement()
	: name("")
	, contents("")
{
}

XMLElement::XMLElement(const std::string& name)
	: name(name)
	, contents("")
{

}

XMLElement::XMLElement(const std::string& name, const std::map<std::string, XMLAttribute>& attributes)
	: name(name)
	, attributes(attributes)
{

}

XMLElement::XMLElement(const std::string& name, const std::map<std::string, XMLAttribute>& attributes, const std::string& contents)
	: name(name)
    , contents(contents)
	, attributes(attributes)
{

}

XMLElement::~XMLElement()
{
}

std::string XMLElement::GetName() const
{
	return name;
}

std::string XMLElement::GetContents() const
{
	return contents;
}

XMLAttribute XMLElement::GetAttribute(const std::string& name) const
{
	return attributes.at(name);
}

bool XMLElement::AttributeExists(const std::string& name) const
{
	return attributes.find(name) != attributes.end();
}

std::vector<XMLElement> XMLElement::GetChildren(const std::string& name) const
{
	return children.at(name);
}

bool XMLElement::ChildExists(const std::string& name) const
{
	return children.find(name) != children.end();
}

std::string XMLElement::ToFormattedString() const
{
	std::string returnString = name + ": \n";

	for(auto pair : attributes)
		returnString += "\tAttribute: " + pair.first + "=\"" + pair.second.GetValueAsString() + "\"\n"; //\tAttribute: <attributeName>="<attributeValue>"\n

	if(contents.size() > 0)
		returnString += "\tContent: \"" + contents + "\"\n"; //\tContent: "<content>"\n

	int i = 0;

	for(auto pair : children)
	{
		i++;

		for(auto child : pair.second)
			returnString += child.ToFormattedStringRec(i);

		i--;
	}

	return returnString;
}

std::string XMLElement::ToFormattedStringRec(int indent) const
{
	std::string indentString = "";
	
	for(int i = 0; i < indent; i++)
		indentString += "\t";

	std::string returnString = indentString + name + ": \n";

	for(auto pair : attributes)
		returnString += indentString + "\tAttribute: " + pair.first + "=\"" + pair.second.GetValueAsString() + "\"\n"; //\t<attributeName>="<attributeValue>"\n

	if(contents.size() > 0)
		returnString += indentString + "\tContent: \"" + contents + "\"\n";

	int i = indent;

	for(auto pair : children)
	{
		i++;

		for(auto child : pair.second)
			returnString += child.ToFormattedStringRec(i);

		i--;
	}

	return returnString;
}
