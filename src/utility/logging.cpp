#include <utility/logging.hpp>

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

		void set_red() {
			SetConsoleTextAttribute(stdout_, FOREGROUND_RED | FOREGROUND_INTENSITY);
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

		void set_red() {
			printf("\e[31m");
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

			auto now = steady_clock::now();
			auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

			char buffer[2048];
			vsnprintf(buffer, sizeof(buffer), format, args);
			AddMessage(timestamp.count(), buffer);

			s_consoleColors.SetGrey();
			printf("[%6d.%03d]\t", (int)(timestamp.count() / 1000), (int)(timestamp.count() % 1000));

			s_consoleColors.set_red();
			printf("ERROR:\t");

			s_consoleColors.SetWhite();
			puts(buffer);

			s_consoleColors.SetGrey();

			va_end(args);
		}

		void Log(const char* format, ...) {
			va_list args;
			va_start(args, format);

			auto now = steady_clock::now();
			auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

			char buffer[2048];
			vsnprintf(buffer, sizeof(buffer), format, args);
			AddMessage(timestamp.count(), buffer);

			s_consoleColors.SetGrey();
			printf("[%6d.%03d]\t", (int)(timestamp.count() / 1000), (int)(timestamp.count() % 1000));

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

		steady_clock::time_point start = steady_clock::now();

		std::list<LogEntry> logEntries;
		std::mutex mutex;
	};

	static Logger s_log;
	ILogger* g_log = &s_log;
}
