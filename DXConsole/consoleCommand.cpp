#include "consoleCommand.h"

ConsoleCommand::ConsoleCommand(const std::string& name, bool forceStringArguments)
	: name(name)
	, forceStringArguments(forceStringArguments)
{
}

std::string ConsoleCommand::GetName() const
{
	return name;
}

std::string ConsoleCommand::GetHelpUsageExample() const
{
	return GetHelp() + "\nUsage: " + GetUsage() + "\nExample: " + GetExample();
}

bool ConsoleCommand::GetForceStringArguments() const
{
	return forceStringArguments;
}
