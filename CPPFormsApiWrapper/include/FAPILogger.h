#ifndef FAPILOGGER_H
#define FAPILOGGER_H

#include "dllmain.h"
#include <string>
#include <fstream>
#include "easylogger.h"

namespace CPPFAPIWrapper {

	class FAPILogger
	{
	public:
		CPPFAPIWRAPPER static void flush();
		CPPFAPIWRAPPER static void disable();
		CPPFAPIWRAPPER static void enable();
		CPPFAPIWRAPPER static void setLevel(easylogger::LogLevel _level);
		CPPFAPIWRAPPER static void changePath(std::string _logpath);

		CPPFAPIWRAPPER static void fatal(std::string _str);
		CPPFAPIWRAPPER static void error(std::string _str);
		CPPFAPIWRAPPER static void warn(std::string _str);
		CPPFAPIWRAPPER static void info(std::string _str);
		CPPFAPIWRAPPER static void debug(std::string _str);
		CPPFAPIWRAPPER static void trace(std::string _str);

		CPPFAPIWRAPPER static std::string getFilepath();
		CPPFAPIWRAPPER static bool isEnabled();
		CPPFAPIWRAPPER static easylogger::LogLevel getLogLevel();
	private:
		CPPFAPIWRAPPER static std::string getTimestamp();

		static std::string filepath;
		static std::ofstream stream;
		static bool is_enabled;
		static easylogger::LogLevel level;
		static easylogger::Logger logger;
	};
}

#define TRACE_FNC(_msg) FAPILogger::trace(std::string(__FILE__).substr(std::string(__FILE__).rfind('\\')) + " (" + std::string(__FUNCTION__) + " " + std::to_string(__LINE__) + "): " + _msg);

#endif // FAPILOGGER_H
