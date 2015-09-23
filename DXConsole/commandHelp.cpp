#include "commandHelp.h"

#include "consoleCommandManager.h"

CommandHelp::CommandHelp(const std::string& name)
	: ConsoleCommand(name, true)
{
}

Argument CommandHelp::Execute(const ContextPointers* const contextPointers, const std::vector<Argument>& arguments)
{
	Argument returnArgument;

	if(arguments.size() == 0)
		returnArgument = GetHelpUsageExample();
	else if(arguments.size() == 1)
	{
		Argument frontArgument = arguments.front();

		auto parenIndex = frontArgument.values.front().find('(');
		if(parenIndex != frontArgument.values.front().npos)
			frontArgument.values.front().erase(parenIndex);

		if(contextPointers->commandManager->GetCommand(frontArgument.values.front()) != nullptr)
			returnArgument = "Printing help for " + arguments.front() + ":\n" + contextPointers->commandManager->GetCommand(frontArgument.values.front())->GetHelpUsageExample();
		else
			returnArgument = "Couldn't print help for command \"" + frontArgument.values.front() + "\" since there is no such command";
	}
	else
		returnArgument = "Expected one or zero arguments, got " + std::to_string(arguments.size());

	returnArgument.origin = "HelpCommand";
	return returnArgument;
}

std::string CommandHelp::GetHelp() const
{
	return "Prints some helpful text so that you know what a command does and how to use it (just like this!)";
}

std::string CommandHelp::GetUsage() const
{
	return GetName() + "(<command>)\nWhere command is the command you want help with";
}

std::string CommandHelp::GetExample() const
{
	return "You seem to have this down already";
}