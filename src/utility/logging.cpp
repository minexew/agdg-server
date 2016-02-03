#include <utility/logging.hpp>

#include <list>
#include <mutex>
#include <string>

#include <cstdint>
#include <cstdio>
#include <ctime>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace agdg {
#ifdef _WIN32
	class ConsoleColors {
	public:
		ConsoleColors() {
			stdout_ = GetStdHandle(STD_OUTPUT_HANDLE);
		}

		void SetWhite() {
			SetConsoleTextAttribute(stdout_, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		}

		void SetGrey() {
			SetConsoleTextAttribute(stdout_, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		}

	private:
		HANDLE stdout_;
	};
#else
	class ConsoleColors {
	public:
		ConsoleColors() {
		}

		void SetWhite() {
			printf("\e[97m");
		}

		void SetGrey() {
			printf("\e[37m");
		}
	};	
#endif

	static ConsoleColors s_consoleColors;

	struct LogEntry {
		LogTimestamp timestamp;
		std::string message;
	};

	class Logger : public ILogger {
	public:
		void Log(const char* format, ...) {
			va_list args;
			va_start(args, format);

			auto ts = clock();
			float t = ts / (float)CLOCKS_PER_SEC;

			char buffer[2048];
			vsnprintf(buffer, sizeof(buffer), format, args);
			AddMessage(ts, buffer);

			s_consoleColors.SetGrey();
			printf("[%9.3f]\t", t);

			s_consoleColors.SetWhite();
			puts(buffer);

			s_consoleColors.SetGrey();

			va_end(args);
		}

		void GetAllMessages(std::function<void(LogTimestamp, const std::string&)> callback) {
			std::lock_guard<std::mutex> lock(mutex);

			for (auto entry : logEntries) {
				callback(entry.timestamp, entry.message);
			}
		}

	private:
		void AddMessage(LogTimestamp timestamp, const char* message) {
			std::lock_guard<std::mutex> lock(mutex);

			if (logEntries.size() >= kMaxLogSize)
				logEntries.pop_front();

			logEntries.push_back(LogEntry{ timestamp, message });
		}

		enum { kMaxLogSize = 50 };

		std::list<LogEntry> logEntries;
		std::mutex mutex;
	};

	static Logger s_log;
	ILogger* g_log = &s_log;
}
