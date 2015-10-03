#ifndef OPENGLWINDOW_COMMANDGETSET_H
#define OPENGLWINDOW_COMMANDGETSET_H

#include "consoleVariable.h"

#include <sstream>

template<typename T>
class CommandGetSet
	: public ConsoleVariable
{
public:
	CommandGetSet(std::string name, T* value)
		: ConsoleVariable(name, false)
		, value(value)
	{ }

	virtual ~CommandGetSet() {}

	virtual Argument Execute(const ContextPointers* const contextPointers, const std::vector<Argument>& arguments) override
	{
		Argument returnArgument;
		returnArgument.origin = name;

		if(arguments.empty())
		{
			//Get
			*value >> returnArgument;
		}
		else
		{
			//Combine all arguments into one big argument
			//This means that only two overloads are neede for custom types (T):
			//T >> Argument
			//Argument >> T
			Argument tempArgument;

			for(const Argument& argument : arguments)
			{
				for(const std::string& value : argument.values)
					tempArgument.values.emplace_back(value);
			}

			//Set
			if(!(tempArgument >> *value))
				returnArgument.values.emplace_back("Couldn't insert \"" + tempArgument + "\" into value. Check your input and/or data types.");
			else
			{
				//Use sstream since it already has overloads for >> for primitive data types
				*value >> returnArgument;
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
		*value >> tempArgument;

		std::string tempString;
		tempArgument >> tempString;

		return tempString;
	}

protected:
	T* value;
};

#endif //OPENGLWINDOW_COMMANDGETSET_H
