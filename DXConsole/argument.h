#ifndef OPENGLWINDOW_PARAMETER_H
#define OPENGLWINDOW_PARAMETER_H

//This file is massive... Maybe one day I will figure out a way to shrink it
//Made it smaller, but it's still pretty big

#include <string>
#include <sstream>
#include <vector>
#include <limits>

#include <sstream>

#include <DirectXMath.h>

namespace
{
	const std::string TYPE_TO_STRING[9] = { "none", "bool", "int32", "int64", "uint64", "float", "double", "string", "unknown" };
}

struct Argument
{
	enum class TYPE { NONE, BOOL, INT32, INT64, UINT64, FLOAT, DOUBLE, STRING, UNKNOWN };

	Argument() 
		: type(TYPE::NONE)
	{}
	Argument(const std::string& text)
		: type(TYPE::STRING)
	{
		values.emplace_back(text);
	}
	Argument(const char* text)
		: Argument(std::string(text))
	{}
	~Argument() = default;

	Argument& operator=(const std::string& rhs)
	{
		origin = "";
		values.emplace_back(rhs);
		type = TYPE::STRING;

		return *this;
	}

	Argument operator+(const Argument& rhs)
	{
		auto pair = ConvertToCompatibleTypes(rhs);

		if(pair.first.values.size() != pair.second.values.size())
		{
			if(pair.first.values.size() == 1)
				return pair.first; //1 + (1,2,3) doens't really make sense, so don't do anything
			else if(pair.second.values.size() == 1)
				pair.second = std::move(PadValues(pair.second, pair.first.values.size())); //(1,2,3) + 1 just adds 1 to each index
		}

		Argument returnArgument;
		returnArgument.type = pair.first.type;
		returnArgument.origin = pair.first.values.front() + " - " + pair.second.values.front();

		returnArgument.values.resize(pair.first.values.size());

		switch (pair.first.type)
		{
		case TYPE::BOOL:
		{
			//Convert to double because then it works whether it's a double, int, or whatever
			for(std::vector<std::string>::size_type i = 0, end = pair.first.values.size(); i < end; ++i)
			{
				double addVal = ParseType<double>(pair.second.values[i]);
				//Adding > 0 to a bool will always make it 1
				if(addVal > 0.0)
					returnArgument.values[i] = "1";
			}
			break;
		}
		case TYPE::INT32:
			returnArgument.values = std::move(AddTypeAndConvertToString<int32_t>(pair.first.values, pair.second.values));
			break;
		case TYPE::INT64:
			returnArgument.values = std::move(AddTypeAndConvertToString<int64_t>(pair.first.values, pair.second.values));
			break;
		case TYPE::UINT64:
			returnArgument.values = std::move(AddTypeAndConvertToString<uint32_t>(pair.first.values, pair.second.values));
			break;
		case TYPE::FLOAT:
			returnArgument.values = std::move(AddTypeAndConvertToString<float>(pair.first.values, pair.second.values));
			break;
		case TYPE::DOUBLE:
			returnArgument.values = std::move(AddTypeAndConvertToString<double>(pair.first.values, pair.second.values));
			break;
		case TYPE::STRING:
			for(std::vector<std::string>::size_type i = 0, end = pair.first.values.size(); i < end; ++i)
				returnArgument.values[i] = pair.first.values[i] + pair.second.values[i];
			break;
		default:
			//Nothing to do for the rest
			break;
		}

		return returnArgument;
	}

	Argument operator-(const Argument& rhs)
	{
		//Check operator+ for comments

		auto pair = ConvertToCompatibleTypes(rhs);

		if(pair.first.values.size() != pair.second.values.size())
		{
			if(pair.first.values.size() == 1)
				return pair.first;
			else if(pair.second.values.size() == 1)
				pair.second = std::move(PadValues(pair.second, pair.first.values.size()));
		}

		Argument returnArgument;
		returnArgument.type = pair.first.type;
		returnArgument.origin = pair.first.values.front() + " - " + pair.second.values.front();

		returnArgument.values.resize(pair.first.values.size());

		switch(pair.first.type)
		{
			case TYPE::BOOL:
			{
				for(std::vector<std::string>::size_type i = 0, end = pair.first.values.size(); i < end; ++i)
				{
					double addVal = ParseType<double>(pair.second.values[i]);

					if(addVal > 0.0)
						returnArgument.values[i] = "0";
				}
				break;
			}
			case TYPE::INT32:
				returnArgument.values = std::move(SubtractTypeAndConvertToString<int32_t>(pair.first.values, pair.second.values));
				break;
			case TYPE::INT64:
				returnArgument.values = std::move(SubtractTypeAndConvertToString<int64_t>(pair.first.values, pair.second.values));
				break;
			case TYPE::UINT64:
				returnArgument.values = std::move(SubtractTypeAndConvertToString<uint64_t>(pair.first.values, pair.second.values));
				break;
			case TYPE::FLOAT:
				returnArgument.values = std::move(SubtractTypeAndConvertToString<float>(pair.first.values, pair.second.values));
				break;
			case TYPE::DOUBLE:
				returnArgument.values = std::move(SubtractTypeAndConvertToString<double>(pair.first.values, pair.second.values));
				break;
			default:
				//No string since -= isn't an operator for strings in C++
				break;
		}

		return returnArgument;
	}

	Argument operator*(const Argument& rhs)
	{
		//Check operator+ for comments

		auto pair = ConvertToCompatibleTypes(rhs);

		if(pair.first.values.size() != pair.second.values.size())
		{
			if(pair.first.values.size() == 1)
				return pair.first;
			else if(pair.second.values.size() == 1)
				pair.second = std::move(PadValues(pair.second, pair.first.values.size()));
		}

		Argument returnArgument;
		returnArgument.type = pair.first.type;
		returnArgument.origin = pair.first.values.front() + " * " + pair.second.values.front();

		returnArgument.values.resize(pair.first.values.size());

		switch(pair.first.type)
		{
			case TYPE::BOOL:
			{
				for(std::vector<std::string>::size_type i = 0, end = pair.first.values.size(); i < end; ++i)
				{
					double addVal = ParseType<double>(pair.second.values[i]);

					if(addVal == 0.0)
						returnArgument.values[i] = "0";
				}
				break;
			}
			case TYPE::INT32:
				returnArgument.values = std::move(MultiplyTypeAndConvertToString<int32_t>(pair.first.values, pair.second.values));
				break;
			case TYPE::INT64:
				returnArgument.values = std::move(MultiplyTypeAndConvertToString<int64_t>(pair.first.values, pair.second.values));
				break;
			case TYPE::UINT64:
				returnArgument.values = std::move(MultiplyTypeAndConvertToString<uint64_t>(pair.first.values, pair.second.values));
				break;
			case TYPE::FLOAT:
				returnArgument.values = std::move(MultiplyTypeAndConvertToString<float>(pair.first.values, pair.second.values));
				break;
			case TYPE::DOUBLE:
				returnArgument.values = std::move(MultiplyTypeAndConvertToString<double>(pair.first.values, pair.second.values));
				break;
			default:
				//No string since -= isn't an operator for strings in C++
				break;
		}

		return returnArgument;
	}

	Argument operator/(const Argument& rhs)
	{
		//Check operator+ for comments

		auto pair = ConvertToCompatibleTypes(rhs);

		if(pair.first.values.size() != pair.second.values.size())
		{
			if(pair.first.values.size() == 1)
				return pair.first;
			else if(pair.second.values.size() == 1)
				pair.second = std::move(PadValues(pair.second, pair.first.values.size()));
		}

		Argument returnArgument;
		returnArgument.type = pair.first.type;
		returnArgument.origin = pair.first.values.front() + " / " + pair.second.values.front();

		returnArgument.values.resize(pair.first.values.size());

		switch(pair.first.type)
		{
			case TYPE::INT32:
				returnArgument.values = std::move(DivideTypeAndConvertToString<int32_t>(pair.first.values, pair.second.values));
				break;
			case TYPE::INT64:
				returnArgument.values = std::move(DivideTypeAndConvertToString<int64_t>(pair.first.values, pair.second.values));
				break;
			case TYPE::UINT64:
				returnArgument.values = std::move(DivideTypeAndConvertToString<uint64_t>(pair.first.values, pair.second.values));
				break;
			case TYPE::FLOAT:
				returnArgument.values = std::move(DivideTypeAndConvertToString<float>(pair.first.values, pair.second.values));
				break;
			case TYPE::DOUBLE:
				returnArgument.values = std::move(DivideTypeAndConvertToString<double>(pair.first.values, pair.second.values));
				break;
			default:
				//Who would ever divide a bool?
				break;
		}

		return returnArgument;
	}

	explicit operator std::string() const
	{
		if(values.size() == 0)
			return "";

		std::string asString;

		if(values.size() == 1)
			asString += values.front();
		else
		{
			asString += "[";

			for(auto value : values)
				asString += value + ", ";

			asString.erase(asString.size() - 2);

			asString += "]";
		}

		return asString;
	}

	template<typename T>
	T ParseType(const std::string& value)
	{
		std::stringstream sstream;
		T returnValue;

		sstream << value;
		sstream >> returnValue;

		return returnValue;
	}

	//"origin" is used to represent where "value" came from.
	//For instance, when the GetSet function is called the origin will be the variable's name.
	//When the Print function is called the origin will be "CommandPrint"
	std::string origin;
	std::vector<std::string> values;
	TYPE type;

private:
	std::pair<Argument, Argument> ConvertToCompatibleTypes(const Argument& other)
	{
		std::pair<Argument, Argument> returnPair(*this, other);

		if(type == TYPE::FLOAT || type == TYPE::DOUBLE)
		{
			if(other.type == TYPE::DOUBLE)
				returnPair.first.type = TYPE::DOUBLE;
			else
				returnPair.second.type = type;
		}
		else if(other.type == TYPE::FLOAT || other.type == TYPE::DOUBLE)
		{
			if(type == TYPE::DOUBLE)
				returnPair.second.type = TYPE::DOUBLE;
			else
				returnPair.first.type = other.type;
		}
		else
		{
			switch(type)
			{
			case TYPE::NONE:
				returnPair.first.type = other.type;
				break;
			case TYPE::BOOL:
				returnPair.second.type = TYPE::BOOL;
				break;
			case TYPE::INT32:
				if(other.type == TYPE::INT64 || other.type == TYPE::UINT64)
					returnPair.first.type = other.type;
				else
					returnPair.second.type = TYPE::INT32;
				break;
			case TYPE::INT64:
				if(other.type == TYPE::UINT64)
					returnPair.first.type = TYPE::UINT64;
				else
					returnPair.second.type = TYPE::INT64;
				break;
			case TYPE::UINT64:
				returnPair.second.type = TYPE::UINT64;
				break;
			case TYPE::STRING:
				returnPair.second.type = TYPE::STRING;
				break;
			case TYPE::UNKNOWN:
				returnPair.second.type = TYPE::UNKNOWN;
				break;
			default:
				break;
			}
		}

		return returnPair;
	}
	Argument PadValues(Argument argument, std::vector<std::string>::size_type count)
	{
		values.reserve(count);

		for(std::vector<std::string>::size_type i = 1; i < count; ++i)
			values.emplace_back(values.front());

		return argument;
	}

	template<typename T>
	std::vector<std::string> AddTypeAndConvertToString(std::vector<std::string>& lhs, std::vector<std::string>& rhs)
	{
		std::vector<std::string> returnVector;
		returnVector.reserve(lhs.size());

		for(std::vector<std::string>::size_type i = 0, end = lhs.size(); i < end; ++i)
		{
			T first = ParseType<T>(lhs[i]);
			T second = ParseType<T>(rhs[i]);

			returnVector.emplace_back(std::to_string(first + second));
		}

		return returnVector;
	}

	template<typename T>
	std::vector<std::string> SubtractTypeAndConvertToString(std::vector<std::string>& lhs, std::vector<std::string>& rhs)
	{
		std::vector<std::string> returnVector;
		returnVector.reserve(lhs.size());

		for(std::vector<std::string>::size_type i = 0, end = lhs.size(); i < end; ++i)
		{
			T first = ParseType<T>(lhs[i]);
			T second = ParseType<T>(rhs[i]);

			returnVector.emplace_back(std::to_string(first - second));
		}

		return returnVector;
	}

	template<typename T>
	std::vector<std::string> MultiplyTypeAndConvertToString(std::vector<std::string>& lhs, std::vector<std::string>& rhs)
	{
		std::vector<std::string> returnVector;
		returnVector.reserve(lhs.size());

		for(std::vector<std::string>::size_type i = 0, end = lhs.size(); i < end; ++i)
		{
			T first = ParseType<T>(lhs[i]);
			T second = ParseType<T>(rhs[i]);

			returnVector.emplace_back(std::to_string(first * second));
		}

		return returnVector;
	}

	template<typename T>
	std::vector<std::string> DivideTypeAndConvertToString(std::vector<std::string>& lhs, std::vector<std::string>& rhs)
	{
		std::vector<std::string> returnVector;
		returnVector.reserve(lhs.size());

		for(std::vector<std::string>::size_type i = 0, end = lhs.size(); i < end; ++i)
		{
			T first = ParseType<T>(lhs[i]);
			T second = ParseType<T>(rhs[i]);

			if(second != static_cast<T>(0))
				returnVector.emplace_back(std::to_string(first / second));
			else
				returnVector.emplace_back("Division by zero: " + std::to_string(first) + "/" + std::to_string(second));
		}

		return returnVector;
	}
};

namespace
{
	template<typename T>
	bool ExtractFromArgument(const Argument& lhs, T& rhs)
	{
		if(lhs.values.size() != 1)
			return false;

		std::stringstream sstream;
		sstream << lhs.values.front();

		sstream >> rhs;

		return true;
	}
}

//Console performs bounds checking (INT_MAX etc.), so there's no need to do it here
//Operators to convert a list of argument to primitive data types as well as string concats
inline bool operator>>(const Argument& lhs, bool& rhs)
{
	if(lhs.values.size() != 1)
		return false;

	rhs = std::atoi(&lhs.values.front().c_str()[0]) > 0;

	return true;
}

inline bool operator>>(const Argument& lhs, int8_t& rhs)
{
	return ExtractFromArgument(lhs, rhs);
}

inline bool operator>>(const Argument& lhs, uint8_t& rhs)
{
	return ExtractFromArgument(lhs, rhs);
}

inline bool operator>>(const Argument& lhs, int16_t& rhs)
{
	return ExtractFromArgument(lhs, rhs);
}

inline bool operator>>(const Argument& lhs, uint16_t& rhs)
{
	return ExtractFromArgument(lhs, rhs);
}

inline bool operator>>(const Argument& lhs, int32_t& rhs)
{
	return ExtractFromArgument(lhs, rhs);
}

inline bool operator>>(const Argument& lhs, uint32_t& rhs)
{
	return ExtractFromArgument(lhs, rhs);
}

inline bool operator>>(const Argument& lhs, int64_t& rhs)
{
	return ExtractFromArgument(lhs, rhs);
}

inline bool operator>>(const Argument& lhs, uint64_t& rhs)
{
	return ExtractFromArgument(lhs, rhs);
}

inline bool operator>>(const Argument& lhs, float& rhs)
{
	return ExtractFromArgument(lhs, rhs);
}

inline bool operator>>(const Argument& lhs, double& rhs)
{
	return ExtractFromArgument(lhs, rhs);
}

inline void operator>>(const Argument& lhs, std::string& rhs)
{
	rhs = std::string(lhs);
}

inline void operator>>(bool lhs, Argument& rhs)
{
	rhs.values.emplace_back(std::to_string(lhs));
	rhs.type = Argument::TYPE::BOOL;
}

inline void operator>>(int8_t lhs, Argument& rhs)
{
	rhs.values.emplace_back(std::to_string(lhs));
	rhs.type = Argument::TYPE::INT32;
}

inline void operator>>(uint8_t lhs, Argument& rhs)
{
	rhs.values.emplace_back(std::to_string(lhs));
	rhs.type = Argument::TYPE::INT32;
}

inline void operator>>(int16_t lhs, Argument& rhs)
{
	rhs.values.emplace_back(std::to_string(lhs));
	rhs.type = Argument::TYPE::INT32;
}

inline void operator>>(uint16_t lhs, Argument& rhs)
{
	rhs.values.emplace_back(std::to_string(lhs));
	rhs.type = Argument::TYPE::INT32;
}

inline void operator>>(int32_t lhs, Argument& rhs)
{
	rhs.values.emplace_back(std::to_string(lhs));
	rhs.type = Argument::TYPE::INT32;
}

inline void operator>>(uint32_t lhs, Argument& rhs)
{
	rhs.values.emplace_back(std::to_string(lhs));
	rhs.type = Argument::TYPE::INT64;
}

inline void operator>>(int64_t lhs, Argument& rhs)
{
	rhs.values.emplace_back(std::to_string(lhs));
	rhs.type = Argument::TYPE::INT64;
}

inline void operator>>(uint64_t lhs, Argument& rhs)
{
	rhs.values.emplace_back(std::to_string(lhs));
	rhs.type = Argument::TYPE::UINT64;
}

inline void operator>>(float lhs, Argument& rhs)
{
	rhs.values.emplace_back(std::to_string(lhs));
	rhs.type = Argument::TYPE::FLOAT;
}

inline void operator>>(double lhs, Argument& rhs)
{
	rhs.values.emplace_back(std::to_string(lhs));
	rhs.type = Argument::TYPE::DOUBLE;
}

inline void operator>>(const std::string& lhs, Argument& rhs)
{
	rhs.values.emplace_back(lhs);
	rhs.type = Argument::TYPE::STRING;
}


inline bool operator>>(const Argument& lhs, DirectX::XMFLOAT2& rhs)
{
	if(lhs.values.size() != 2)
		return false;

	rhs.x = std::stof(lhs.values[0]);
	rhs.y = std::stof(lhs.values[1]);

	return true;
}

inline bool operator>>(const DirectX::XMFLOAT2& lhs, Argument& rhs)
{
	rhs.values.clear();

	rhs.type = Argument::TYPE::FLOAT;

	rhs.values.emplace_back(std::to_string(lhs.x));
	rhs.values.emplace_back(std::to_string(lhs.y));

	return true;
}

inline bool operator>>(const Argument& lhs, DirectX::XMFLOAT3& rhs)
{
	if(lhs.values.size() != 3)
		return false;

	rhs.x = std::stof(lhs.values[0]);
	rhs.y = std::stof(lhs.values[1]);
	rhs.z = std::stof(lhs.values[2]);

	return true;
}

inline bool operator>>(const DirectX::XMFLOAT3& lhs, Argument& rhs)
{
	rhs.values.clear();

	rhs.type = Argument::TYPE::FLOAT;

	rhs.values.emplace_back(std::to_string(lhs.x));
	rhs.values.emplace_back(std::to_string(lhs.y));
	rhs.values.emplace_back(std::to_string(lhs.z));

	return true;
}

inline std::string operator+(const std::string& lhs, const Argument& rhs)
{
	std::string temp = lhs;

	rhs >> temp;

	return temp;
}

inline std::string operator+(const Argument& lhs, const std::string& rhs)
{
	std::string temp;

	lhs >> temp;
	temp += rhs;

	return temp;
}

inline std::string operator+=(std::string lhs, const Argument& rhs)
{
	lhs = lhs + rhs;
	return lhs;
}

inline std::string operator+=(const Argument& lhs, std::string rhs)
{
	rhs = lhs + rhs;
	return rhs;
}


#endif //OPENGLWINDOW_PARAMETER_H
