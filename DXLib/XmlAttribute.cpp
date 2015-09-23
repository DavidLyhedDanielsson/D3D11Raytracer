#include "xmlAttribute.h"

XMLAttribute::XMLAttribute()
	: name("")
	, value("")
{
}

XMLAttribute::XMLAttribute(const std::string& name, const std::string& value)
	:name(name)
	,value(value)
{

}

XMLAttribute::~XMLAttribute()
{
}

std::string XMLAttribute::GetName() const
{
	return name;
}

short XMLAttribute::GetValueAsShort() const
{
	return static_cast<short>(stoi(value));
}

int XMLAttribute::GetValueAsInt() const
{
	return stoi(value);
}

unsigned int XMLAttribute::GetValueAsUnsignedInt() const
{
	return static_cast<unsigned int>(stol(value));
}

long XMLAttribute::GetValueAsLong() const
{
	return stol(value);
}

unsigned long XMLAttribute::GetValueAsUnsignedLong() const
{
	return stoul(value);
}

long long XMLAttribute::GetValueAsLongLong() const
{
	return stoll(value);
}

unsigned long long XMLAttribute::GetValueAsUnsignedLongLong() const
{
	return stoull(value);
}

float XMLAttribute::GetValueAsFloat() const
{
	return stof(value);
}

double XMLAttribute::GetValueAsDouble() const
{
	return stod(value);
}

char XMLAttribute::GetValueAsChar() const
{
	return static_cast<char>(stoi(value));
}

unsigned char XMLAttribute::GetValueAsUnsignedChar() const
{
	return static_cast<unsigned char>(stoi(value));
}

std::string XMLAttribute::GetValueAsString() const
{
	return value;
}
