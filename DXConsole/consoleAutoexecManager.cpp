#include "consoleAutoexecManager.h"

#include <fstream>

#include <DXLib/logger.h>

ConsoleAutoexecManager::ConsoleAutoexecManager()
	: autoexecLoaded(false)
	, autoexecFound(false)
{}

void ConsoleAutoexecManager::FunctionExecuted(const std::string& function, const std::string& arguments)
{
	if(autoexecWatchesVariables.count(function) > 0)
		autoexecWatchesVariables[function] = arguments;
	else if(autoexecWatchesFunctions.count(function) > 0)
		autoexecWatchesFunctions[function] = arguments;
}

bool ConsoleAutoexecManager::AddAutoexecWatch(const std::string& command, ConsoleCommandManager& commandManager)
{
	if(commandManager.GetCommand(command) == nullptr)
		return false;

	if(autoexecWatchesVariables.count(command) == 0
		&& autoexecWatchesFunctions.count(command) == 0)
	{
		auto variable = commandManager.GetVariable(command);

		if(variable == nullptr)
			autoexecWatchesFunctions.emplace(command, "");
		else
			autoexecWatchesVariables.emplace(command, variable->GetValue());
	}

	return true;
}

bool ConsoleAutoexecManager::RemoveAutoexecWatch(const std::string& command)
{
	if(autoexecWatchesVariables.count(command) > 0)
	{
		autoexecWatchesVariables.erase(command);
		removedWatches.emplace(command);
		return true;
	}
	else if(autoexecWatchesFunctions.count(command) > 0)
	{
		autoexecWatchesFunctions.erase(command);
		removedWatches.emplace(command);
		return true;
	}

	return false;
}

std::vector<std::pair<std::string, std::string>> ConsoleAutoexecManager::GetWatches()
{
	std::vector<std::pair<std::string, std::string>> returnVector;

	returnVector.reserve(autoexecWatchesVariables.size() + autoexecWatchesFunctions.size());

	for(const auto& pair : autoexecWatchesVariables)
		returnVector.emplace_back(pair.first, pair.second);

	for(const auto& pair : autoexecWatchesFunctions)
		returnVector.emplace_back(pair.first, pair.second);

	return returnVector;
}

std::vector<std::pair<std::string, std::string>> ConsoleAutoexecManager::GetVariableWatches()
{
	std::vector<std::pair<std::string, std::string>> returnVector;

	returnVector.reserve(autoexecWatchesVariables.size());

	for(const auto& pair : autoexecWatchesVariables)
		returnVector.emplace_back(pair.first, pair.second);

	return returnVector;
}

std::vector<std::pair<std::string, std::string>> ConsoleAutoexecManager::GetFunctionWatches()
{
	std::vector<std::pair<std::string, std::string>> returnVector;

	returnVector.reserve(autoexecWatchesFunctions.size());

	for(const auto& pair : autoexecWatchesFunctions)
		returnVector.emplace_back(pair.first, pair.second);

	return returnVector;
}

bool ConsoleAutoexecManager::ParseAutoexec(const std::string& path, ConsoleCommandManager& commandManager)
{
	std::ifstream in(path);

	autoexecLoaded = true;
	if(!in.is_open())
	{
		autoexecFound = false;
		return false;
	}

	std::string line;
	for(int lineNr = 1; std::getline(in, line); ++lineNr)
	{
		line = TrimTextFrontBack(line);

		if(line.empty() || line.compare(0, 2, "//") == 0)
			continue;

		bool addWatch = false;
		if(line.compare(0, 6, "watch ") == 0)
		{
			addWatch = true;
			line = line.substr(6);
		}
		
		std::pair<std::string, std::string> pair = commandManager.ParseFunctionAndArgumentList(line);

		if(addWatch)
		{
			if(!AddAutoexecWatch(pair.first, commandManager))
			{
				Logger::LogLine(LOG_TYPE::WARNING, "Couldn't add \"" + pair.first + "\" from line " + std::to_string(lineNr) + " to autoexec watches; there is no such variable");
				continue;
			}
		}

		try
		{
			commandManager.ExecuteCommand(line);
			FunctionExecuted(pair.first, pair.second);
		}
		catch(std::invalid_argument& ex)
		{
			throw std::invalid_argument("Error during autoexec on line " + std::to_string(lineNr) + ": " + ex.what());
		}
		catch(std::exception& ex)
		{
			throw std::runtime_error("Unknown exception caught during autoexec when trying to execute \"" + line + "\" on line " + std::to_string(lineNr) + ": " + ex.what());
		}
	}

	return true;
}

bool ConsoleAutoexecManager::WriteAutoexec(const std::string& path)
{
	if(!autoexecLoaded)
	{
		Logger::LogLine(LOG_TYPE::WARNING, "WriteAutoexec called without calling ParseAutoexec first");
		return false;
	}

	if(autoexecWatchesVariables.empty() && autoexecWatchesFunctions.empty())
		return true;

	if(!autoexecFound)
		std::ofstream createFile(path);

	std::ofstream out(path + ".tmp", std::ofstream::trunc);
	std::ifstream in(path);

	if(!out.is_open())
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Couldn't create temporary file \"" + path + ".tmp\"" + ", can't write to autoexec");
		return false;
	}
	else if(!in.is_open())
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Couldn't open autoexec file at \"" + path + "\", can't write to autoexec");
		return false;
	}

	std::string line;
	for(int lineNr = 1; std::getline(in, line); ++lineNr)
	{
		line = TrimTextFrontBack(line);

		if(line.empty() || line.compare(0, 2, "//") == 0)
		{
			out << line << '\n';
			continue;
		}

		bool addWatch = false;
		if(line.compare(0, 6, "watch ") == 0)
		{
			addWatch = true;
			line = line.substr(6);
		}

		bool paren = line.find('(') != line.npos;
		auto index = line.find('(');
		if(index == line.npos)
		{
			index = line.find(' ');
			paren = false;
		}

		std::string function = line.substr(0, index);
		if(removedWatches.count(function) != 0)
			continue;

		if(addWatch)
		{
			if(autoexecWatchesVariables.count(function) > 0)
			{
				if(paren)
					out << "watch " << function << "(" << autoexecWatchesVariables[function] << ")" << '\n';
				else
					out << "watch " << function << " " << autoexecWatchesVariables[function] << '\n';

				autoexecWatchesVariables.erase(function);

			}
			else if(autoexecWatchesFunctions.count(function) > 0)
			{
				if(paren)
					out << "watch " << function << "(" << autoexecWatchesFunctions[function] << ")" << '\n';
				else
					out << "watch " << function << " " << autoexecWatchesFunctions[function] << '\n';

				autoexecWatchesFunctions.erase(function);
			}
			else
				out << "watch " << line << '\n';
		}
		else
			out << line << '\n';
	}

	for(auto pair : autoexecWatchesVariables)
		out << "watch " << pair.first << "(" << pair.second << ")" << '\n';

	for(auto pair : autoexecWatchesFunctions)
		out << "watch " << pair.first << "(" << pair.second << ")" << '\n';

	in.close();
	out.close();

	std::remove(path.c_str());
	std::rename((path + ".tmp").c_str(), path.c_str());

	return true;
}

std::string ConsoleAutoexecManager::TrimTextFrontBack(const std::string& text)
{
	size_t firstNotOf = text.find_first_not_of(" \t");
	if(firstNotOf == text.npos)
		return text;

	size_t lastNotOf = text.find_last_not_of(" \t");

	return text.substr(firstNotOf, lastNotOf - firstNotOf + 1);
}
