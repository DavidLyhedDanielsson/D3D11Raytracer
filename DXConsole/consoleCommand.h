#ifndef OPENGLWINDOW_CONSOLECOMMAND_H
#define OPENGLWINDOW_CONSOLECOMMAND_H

#include <string>
#include <vector>

#include "argument.h"
#include "contextPointers.h"

class ConsoleCommand
{
public:
	//ConsoleCommand();
	ConsoleCommand(const std::string& name, bool forceStringArguments);
	virtual ~ConsoleCommand() = default;

	virtual Argument Execute(const ContextPointers* const contextPointers, const std::vector<Argument>& arguments) = 0;

	std::string GetName() const;
	std::string GetHelpUsageExample() const;
	bool GetForceStringArguments() const;

protected:
	std::string name;

	//If this is true then every argument will be a string
	//1 will not be an int, it will be a string
	//1.0f will not be a float, it will be a string
	//SomeFunction will not be a function call, it will become a string
	//An example which uses this is CommandHelp
	bool forceStringArguments;

	virtual std::string GetHelp() const = 0;
	virtual std::string GetUsage() const = 0;
	virtual std::string GetExample() const = 0;
};

#endif //OPENGLWINDOW_CONSOLECOMMAND_H
