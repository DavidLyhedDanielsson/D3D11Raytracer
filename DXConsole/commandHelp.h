#ifndef OPENGLWINDOW_COMMANDHELP_H
#define OPENGLWINDOW_COMMANDHELP_H

#include "consoleCommand.h"

class CommandHelp
		: public ConsoleCommand
{
public:
	CommandHelp(const std::string& name);
	virtual ~CommandHelp() = default;

	Argument Execute(const ContextPointers* const contextPointers,  const std::vector<Argument>& arguments) override;

	std::string GetHelp() const override;
	std::string GetUsage() const override;
	std::string GetExample() const override;
};

#endif //OPENGLWINDOW_COMMANDHELP_H
