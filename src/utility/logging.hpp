#pragma once

#include <functional>

namespace agdg {
	typedef int64_t LogTimestamp;

	class ILogger {
	public:
		virtual void Log(const char* format, ...) = 0;
		virtual void GetAllMessages(std::function<void(LogTimestamp, const std::string&)> callback) = 0;
	};

	extern ILogger* g_log;
}
