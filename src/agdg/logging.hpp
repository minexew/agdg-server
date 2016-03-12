#pragma once

#include <functional>

namespace agdg {
	typedef int64_t LogTimestamp;

	class ILogger {
	public:
		virtual void error(const char* format, ...) = 0;
		virtual void warning(const char* format, ...) = 0;
		virtual void script(const char* format, ...) = 0;

		virtual void get_all_messages(std::function<void(LogTimestamp, const std::string&)> callback) = 0;

		// deprecated
		virtual void Log(const char* format, ...) = 0;
	};

	extern ILogger* g_log;
}
