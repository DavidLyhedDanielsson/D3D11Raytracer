#ifndef Logger_h__
#define Logger_h__

#include <string>
#include <functional>

enum class CONSOLE_LOG_LEVEL : int
{
	NONE = 0x1 //Don't print anything in the console
	, PARTIAL = 0x2 //Print only the message type
	, FULL = 0x4 //Print exactly what is written in the file
	, EXCLUSIVE = 0x8 //Print exclusively to the console
	//Combine any of the types above with the ones below to use Visual Studio's output window
	, DEBUG_STRING = 0x10 //Use binary or to output to Visual Studio's output window
	, DEBUG_STRING_EXCLUSIVE = 0x20 //Use binary or to output to Visual Studio's output window instead of console
};

enum class LOG_TYPE : int
{
	NONE = 0
	, INFO
	, WARNING
	, FATAL
};

class Logger
{
public:
	//static Logger();
	//~Logger();

	static void LogLineWithFileDataF(const char* file, int line, LOG_TYPE logType, const std::string& text);
	static void LogLineWithFileDataF(const char* file, int line, LOG_TYPE logType, const char* text);
	static void LogWithFileDataF(const char* file, int line, LOG_TYPE logType, const std::string& text);
	static void LogWithFileDataF(const char* file, int line, LOG_TYPE logType, const char* text);


	//************************************
	// Method:		LogLine
	// FullName:	Logger::LogLine
	// Access:		public static 
	// Returns:		void
	// Qualifier:	
	// Argument:	LOG_TYPES::LOG_TYPE logType
	// Argument:	const std::string& text
	// Description:	Logs a line of text
	//************************************
	static void LogLine(LOG_TYPE logType, const std::string& text);
	static void LogLine(LOG_TYPE logType, const char* text);

	//************************************
	// Method:		Log
	// FullName:	Logger::Log
	// Access:		public static 
	// Returns:		void
	// Qualifier:	
	// Argument:	LOG_TYPES::LOG_TYPE logType
	// Argument:	const std::string& text
	// Description:	Logs text
	//************************************
	static void Log(LOG_TYPE logType, const std::string& text);
	static void Log(LOG_TYPE logType, const char* text);

	//************************************
	// Method:		ClearLog
	// FullName:	Logger::ClearLog
	// Access:		public 
	// Returns:		void
	// Qualifier:	
	// Description:	Clears the file at the currently set path if it isn't empty
	//************************************
	static void ClearLog();

	//////////////////////////////////////////////////////////////////////////
	//GETTERS
	//////////////////////////////////////////////////////////////////////////

	//************************************
	// Method:		SetSeparatorString
	// FullName:	Logger::SetSeparatorString
	// Access:		public 
	// Returns:		void
	// Qualifier:	
	// Argument:	const std::string& separatorString
	// Description:	When you call log the text will be formatted as such: <LOG_TYPE><separatorString><message>
	//************************************
	static void SetSeparatorString(const std::string& separatorString);

	//************************************
	// Method:		SetOutputDir
	// FullName:	Logger::SetOutputDir
	// Access:		public 
	// Returns:		void
	// Qualifier:	
	// Argument:	const std::string& path
	// Argument:	std::ios_base::openmode openMode
	// Description:	Specifies where to put the log file. No checking is done to make sure path is valid
	//************************************
	static void SetOutputDir(const std::string& path, std::ios_base::openmode openMode);

	//************************************
	// Method:		SetOutputName
	// FullName:	Logger::SetOutputName
	// Access:		public 
	// Returns:		void
	// Qualifier:	
	// Argument:	const std::string& name
	// Argument:	std::ios_base::openmode openMode
	// Description:	Specifies what to call the log file. Include file extension as well
	//************************************
	static void SetOutputName(const std::string& name, std::ios_base::openmode openMode);

	//************************************
	// Method:		SetConsoleLogLevel
	// FullName:	Logger::SetConsoleLogLevel
	// Access:		public 
	// Returns:		void
	// Qualifier:	
	// Argument:	CONSOLE_LOG_LEVELS::CONSOLE_LOG_LEVEL logLevel
	// Description:
	//	CONSOLE_LOG_LEVEL::NONE => No text outputted to console
	//	CONSOLE_LOG_LEVEL::PARTIAL => Only write log type to console
	//	CONSOLE_LOG_LEVEL::FULL => Write everything to console
	//	CONSOLE_LOG_LEVEL::ONLY => Only output to console
	//************************************
	static void SetConsoleLogLevel(CONSOLE_LOG_LEVEL logLevel);

	static void SetCallOnLog(std::function<void(std::string)> function);

	//************************************
	// Method:		PrintStackTrace
	// Argument:	bool print
	// Returns:		void
	// Description:	Sets whether or not to log stack trace whenever Log or LogLine is called (Unix only!)
	//************************************
	static void PrintStackTrace(bool print);

private:
	static CONSOLE_LOG_LEVEL consoleLogLevel;

	static bool printStackTrace;

	//Path to log
	static std::string outPath;
	static std::string outName;

	//When you call log the text will be formatted as such: <LOG_TYPE><separatorString><message>
	static std::string separatorString;

	static std::ios_base::openmode openMode;

	static void Print(std::string message);

	static std::function<void(std::string)> CallOnLog;
};

using T = std::underlying_type_t<CONSOLE_LOG_LEVEL>;

inline CONSOLE_LOG_LEVEL operator|(CONSOLE_LOG_LEVEL lhs, CONSOLE_LOG_LEVEL rhs)
{
	return static_cast<CONSOLE_LOG_LEVEL>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline CONSOLE_LOG_LEVEL& operator|=(CONSOLE_LOG_LEVEL lhs, CONSOLE_LOG_LEVEL rhs)
{
	CONSOLE_LOG_LEVEL bitwiseOr = lhs | rhs;

	return bitwiseOr;
}

inline CONSOLE_LOG_LEVEL operator&(CONSOLE_LOG_LEVEL lhs, CONSOLE_LOG_LEVEL rhs)
{
	return static_cast<CONSOLE_LOG_LEVEL>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

//NOTE: abnormal behaviour
inline bool operator&=(CONSOLE_LOG_LEVEL lhs, CONSOLE_LOG_LEVEL rhs)
{
	CONSOLE_LOG_LEVEL binaryAnd = lhs & rhs;

	return binaryAnd == rhs;
}

#endif // Logger_h__
