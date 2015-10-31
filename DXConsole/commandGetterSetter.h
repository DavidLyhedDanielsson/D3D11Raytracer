#ifndef CommandGetterSetter_h__
#define CommandGetterSetter_h__

#include "consoleVariable.h"

#include <functional>

template<typename T>
class CommandGetterSetter
	: public ConsoleVariable
{
public:
	CommandGetterSetter(const std::string& name, std::function<T(void)> getter, std::function<void(T)> setter)
		: ConsoleVariable(name, false)
		, getter(getter)
		, setter(setter)
	{}
	virtual ~CommandGetterSetter() = default;

	virtual Argument Execute(const ContextPointers* const contextPointers, const std::vector<Argument>& arguments) override
	{
		Argument returnArgument;
		returnArgument.origin = name;

		if(arguments.empty())
		{
			//Get
			getter() >> returnArgument;
		}
		else
		{
			//Combine all arguments into one big argument
			//This means that only two overloads are need for custom types (T):
			//T >> Argument
			//Argument >> T
			Argument tempArgument;

			for(const Argument& argument : arguments)
			{
				for(const std::string& value : argument.values)
					tempArgument.values.emplace_back(value);
			}

			tempArgument.type = arguments.front().type;

			T tempValue;

			//Set
			if(!(tempArgument >> tempValue))
				returnArgument.values.emplace_back("Couldn't insert \"" + tempArgument + "\" into value. Check your input and/or data types.");
			else
			{
				//Use sstream since it already has overloads for >> for primitive data types
				setter(tempValue);

				tempValue >> returnArgument;
				returnArgument.values.front().insert(0, name + " = ");

				returnArgument.type = arguments.size() == 1 ? arguments.front().type : Argument::TYPE::UNKNOWN;
			}
		}

		return returnArgument;
	}

	std::string GetHelp() const override
	{
		return "Gets or sets a C++ variable";
	}
	std::string GetUsage() const override
	{
		return "Get: " + GetName() + "\nSet: " + GetName() + "(<data>)\nWhere data is the new data for the variable";
	}
	std::string GetExample() const override
	{
		return "Get: " + GetName() + "\nSet (assuming data type is float): " + GetName() + "(2.0f)";
	}

	std::string GetValue() const override
	{
		Argument tempArgument;
		getter() >> tempArgument;

		std::string tempString;
		tempArgument >> tempString;

		return tempString;
	}

private:
	std::function<T(void)> getter;
	std::function<void(T)> setter;
};

#endif //CommandGetterSetter_h__