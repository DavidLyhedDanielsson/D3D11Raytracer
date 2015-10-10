#ifndef OPENGLWINDOW_COMMANDCALLMETHOD_H
#define OPENGLWINDOW_COMMANDCALLMETHOD_H

#include "consoleCommand.h"

#include <functional>

class CommandCallMethod
		: public ConsoleCommand
{
public:
	CommandCallMethod(const std::string& name, std::function<Argument(const std::vector<Argument>&)> callMethod, bool forceStringArguments = false);
	virtual ~CommandCallMethod() = default;

	Argument Execute(const ContextPointers* const contextPointers, const std::vector<Argument>& arguments) override;

	std::string GetHelp() const override;
	std::string GetUsage() const override;
	std::string GetExample() const override;

protected:
	std::function<Argument(const std::vector<Argument>&)> callMethod;
};

#endif //OPENGLWINDOW_COMMANDCALLMETHOD_H
