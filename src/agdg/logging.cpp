#include <agdg/logging.hpp>

#include <list>
#include <mutex>
#include <string>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <ctime>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace agdg {
	using std::chrono::steady_clock;

	enum { kMaxLogSize = 50 };

	enum class LogType {
		info,
		script,
		warning,
		error,
	};

#ifdef _WIN32
	class ConsoleColors {
	public:
		ConsoleColors() {
			stdout_ = GetStdHandle(STD_OUTPUT_HANDLE);
		}

		void set_grey() {
			SetConsoleTextAttribute(stdout_, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		}

		void set_red() {
			SetConsoleTextAttribute(stdout_, FOREGROUND_RED | FOREGROUND_INTENSITY);
		}

		void set_yellow() {
			SetConsoleTextAttribute(stdout_, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		}

		void set_white() {
			SetConsoleTextAttribute(stdout_, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		}

	private:
		HANDLE stdout_;
	};
#else
	class ConsoleColors {
	public:
		void reset() {
			printf("\e[0m");
		}

		void set_grey() {
			printf("\e[37m");
		}

		void set_red() {
			printf("\e[31m");
		}

		void set_yellow() {
			printf("\e[33m");
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
		void error(const char* format, ...) {
			va_list args;
			va_start(args, format);
			logv(LogType::error, format, args);
			va_end(args);
		}

		void warning(const char* format, ...) {
			va_list args;
			va_start(args, format);
			logv(LogType::warning, format, args);
			va_end(args);
		}

		void script(const char* format, ...) {
			va_list args;
			va_start(args, format);
			logv(LogType::script, format, args);
			va_end(args);
		}

		void Log(const char* format, ...) {
			va_list args;
			va_start(args, format);
			logv(LogType::info, format, args);
			va_end(args);
		}

		void get_all_messages(std::function<void(LogTimestamp, const std::string&)> callback) {
			std::lock_guard<std::mutex> lock(mutex);

			for (auto entry : logEntries) {
				callback(entry.timestamp, entry.message);
			}
		}

	private:
		void add_message(LogTimestamp timestamp, const char* message) {
			std::lock_guard<std::mutex> lock(mutex);

			if (logEntries.size() >= kMaxLogSize)
				logEntries.pop_front();

			logEntries.push_back(LogEntry{ timestamp, message });
		}

		void logv(LogType type, const char* format, va_list args) {
			auto now = steady_clock::now();
			auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

			char buffer[2048];
			vsnprintf(buffer, sizeof(buffer), format, args);
			add_message(timestamp.count(), buffer);

			s_consoleColors.set_grey();
			printf("[%6d.%03d]\t", (int)(timestamp.count() / 1000), (int)(timestamp.count() % 1000));

			switch (type) {
				case LogType::error:
					s_consoleColors.set_red();
					printf("error:   ");
					break;

				case LogType::warning:
					s_consoleColors.set_yellow();
					printf("warning: ");
					break;

				case LogType::script:
					s_consoleColors.set_yellow();
					printf("script:  ");
					break;

				default:
					;
			}

			s_consoleColors.reset();
			puts(buffer);

			s_consoleColors.set_grey();
		}

		steady_clock::time_point start = steady_clock::now();

		std::list<LogEntry> logEntries;
		std::mutex mutex;
	};

	static Logger s_log;
	ILogger* g_log = &s_log;
}
