#ifndef XMLAttribute_h__
#define XMLAttribute_h__

#include <string>

class XMLAttribute
{
public:
	explicit XMLAttribute();
	explicit XMLAttribute(const std::string& name, const std::string& value);
	~XMLAttribute();

	std::string			GetName() const;

	short				GetValueAsShort() const;
	int					GetValueAsInt() const;
	unsigned int		GetValueAsUnsignedInt() const;
	long				GetValueAsLong() const;
	unsigned long		GetValueAsUnsignedLong() const;
	long long			GetValueAsLongLong() const;
	unsigned long long	GetValueAsUnsignedLongLong() const;
	float				GetValueAsFloat() const;
	double				GetValueAsDouble() const;
	char				GetValueAsChar() const;
	unsigned char		GetValueAsUnsignedChar() const;
	std::string			GetValueAsString() const;
private:
	std::string			name;
	std::string			value;
};

#endif // XMLAttribute_h__
