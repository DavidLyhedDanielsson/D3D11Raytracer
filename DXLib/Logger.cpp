#include "logger.h"

#include <iostream>

#ifndef NO_LOGGER

#include <fstream>
#include <sstream>
#include <windows.h>

#pragma warning(disable : 4091)
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")
#pragma warning(default : 4091)

CONSOLE_LOG_LEVEL Logger::consoleLogLevel = CONSOLE_LOG_LEVEL::FULL | CONSOLE_LOG_LEVEL::DEBUG_STRING;
std::string Logger::outPath = "";
std::string Logger::outName = "log.txt";
std::ios_base::openmode Logger::openMode = std::ios_base::app;
std::string Logger::separatorString = " ";

std::function<void(std::string)> Logger::CallOnLog = nullptr;
bool Logger::printStackTrace = true;

void Logger::LogLineWithFileDataF(const char *file, int line, LOG_TYPE logType, const std::string& text)
{
    std::string message = "File \"";
    message += file;
    message += "\", line ";
    message += std::to_string(line);
    message += ": ";
    message += text;

    LogLine(logType, message);
}

void Logger::LogLineWithFileDataF(const char *file, int line, LOG_TYPE logType, const char* text)
{
    LogLineWithFileDataF(file, line, logType, std::string(text));
}

void Logger::LogWithFileDataF(const char* file, int line, LOG_TYPE logType, const std::string& text)
{
    std::string message = "File \"";
    message += file;
    message += "\", line ";
    message += std::to_string(line);
    message += ": ";
    message += text;

    Log(logType, message);
}

void Logger::LogWithFileDataF(const char* file, int line, LOG_TYPE logType, const char* text)
{
    LogWithFileDataF(file, line, logType, std::string(text));
}

#endif //NO_LOGGER

void Logger::LogLine(LOG_TYPE logType, const std::string& text)
{
    Log(logType, text + "\n");
}

void Logger::LogLine(LOG_TYPE logType, const char* text)
{
    Log(logType, std::string(text) + "\n");
}

#ifndef NO_LOGGER

void Logger::Log(LOG_TYPE logType, const std::string& text)
{
	std::string message("");

	switch(logType)
	{
		case LOG_TYPE::NONE:
			break;
		case LOG_TYPE::INFO:
			message += "[INFO]";
			break;
		case LOG_TYPE::WARNING:
			message += "[WARNING]";
            break;
		case LOG_TYPE::FATAL:
			message += "[FATAL]";
			break;
		default:
			break;
	}

	//Only log error type to console
	//Print message and make sure there is only one \n at the end of it
	if(consoleLogLevel &= CONSOLE_LOG_LEVEL::PARTIAL)
		text[text.size() - 1] != '\n' ? Print(message) : Print(message + "\n");

	if(logType != LOG_TYPE::NONE)
		message += std::string(separatorString.begin(), separatorString.end());

	message += text;

	if(printStackTrace && (logType == LOG_TYPE::WARNING || logType == LOG_TYPE::FATAL))
	{
		message += "Stack trace:\n";

		std::stringstream sstream;

		//http://stackoverflow.com/questions/590160/how-to-log-stack-frames-with-windows-x64
		typedef USHORT(WINAPI *CaptureStackBackTraceType)(__in ULONG, __in ULONG, __out PVOID*, __out_opt PULONG);
		CaptureStackBackTraceType func = (CaptureStackBackTraceType)(GetProcAddress(LoadLibrary("kernel32.dll"), "RtlCaptureStackBackTrace"));

		if(func == NULL)
			return; // WOE 29.SEP.2010

		// Quote from Microsoft Documentation:
		// ## Windows Server 2003 and Windows XP:  
		// ## The sum of the FramesToSkip and FramesToCapture parameters must be less than 63.
		const int kMaxCallers = 62;

		void * callers_stack[kMaxCallers];
		unsigned short frames;
		SYMBOL_INFO * symbol;
		HANDLE process;
		process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);
		frames = (func)(0, kMaxCallers, callers_stack, NULL);
		symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
		symbol->MaxNameLen = 255;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

		const unsigned short  MAX_CALLERS_SHOWN = 6;
		frames = frames < MAX_CALLERS_SHOWN ? frames : MAX_CALLERS_SHOWN;
		for(unsigned int i = 0; i < frames; i++)
		{
			SymFromAddr(process, (DWORD64)(callers_stack[i]), 0, symbol);
			sstream << "    " << symbol->Name << std::endl;
		}

		message += sstream.str();

		free(symbol);
	}

	if((consoleLogLevel &= CONSOLE_LOG_LEVEL::FULL)
		|| (consoleLogLevel &= CONSOLE_LOG_LEVEL::EXCLUSIVE))
	{
		Print(message);

		if(consoleLogLevel &= CONSOLE_LOG_LEVEL::EXCLUSIVE)
		{
			if(CallOnLog != nullptr)
				CallOnLog(message);

			return; //Don't write anything to file
		}
	}
	else if(consoleLogLevel &= CONSOLE_LOG_LEVEL::DEBUG_STRING)
		OutputDebugStringA(message.c_str());
	else if(consoleLogLevel &= CONSOLE_LOG_LEVEL::DEBUG_STRING_EXCLUSIVE)
	{
		OutputDebugStringA(message.c_str());

		if(CallOnLog != nullptr)
			CallOnLog(message);

		return;
	}

	std::ofstream out;
	out.open(outPath + outName, openMode);

	if(out.is_open())
	{
		out.write(&message[0], message.size());
		out.close();
	}

	if(message.back() == '\n')
		message.pop_back();

	if(CallOnLog != nullptr)
		CallOnLog(message);
}

#endif //NO_LOGGER

void Logger::Log(LOG_TYPE logType, const char* text)
{
	Log(logType, std::string(text));
}

#ifndef NO_LOGGER

void Logger::ClearLog()
{
	std::ifstream in(outPath + outName);
	if(!in.is_open())
		return; //File didn't exist

	if(in.peek() == std::ifstream::traits_type::eof())
	{
		in.close();
		return; //File is empty
	}

	in.close();

	std::ofstream out;
	out.open(outPath + outName, std::ios_base::trunc | std::ios_base::out);
	out.close();
}

//////////////////////////////////////////////////////////////////////////
//GETTERS
//////////////////////////////////////////////////////////////////////////

void Logger::SetSeparatorString(const std::string& newSeparatorString)
{
	separatorString = newSeparatorString;
}

void Logger::SetOutputDir(const std::string& path, std::ios_base::openmode newOpenMode)
{
	outPath = path;
	openMode = newOpenMode;
}

void Logger::SetOutputName(const std::string& name, std::ios_base::openmode newOpenMode)
{
	outName = name;
	openMode = newOpenMode;
}

void Logger::SetConsoleLogLevel(CONSOLE_LOG_LEVEL logLevel)
{
	consoleLogLevel = logLevel;
}

void Logger::SetCallOnLog(std::function<void(std::string)> function)
{
	CallOnLog = std::move(function);
}

void Logger::Print(std::string message)
{
    if(consoleLogLevel &= CONSOLE_LOG_LEVEL::DEBUG_STRING)
		OutputDebugStringA(message.c_str());
	
	std::cout << message;
}

void Logger::PrintStackTrace(bool print)
{
	printStackTrace = print;
}

#else

static void Log(LOG_TYPE logType, const std::string& text)
{
	switch(logType)
	{
		case LOG_TYPE::NONE:
			std::cout << text;
			break;
		case LOG_TYPE::INFO:
			std::cout << "[INFO]" << text;
			break;
		case LOG_TYPE::WARNING:
			std::cout << "[WARNING]" << text;
			break;
		case LOG_TYPE::FATAL:
			std::cout << "[FATAL]" << text;
			break;
	}
}

#endif //NO_LOGGER