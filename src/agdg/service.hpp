#pragma once

#include <string>

namespace agdg {
	class IService {
	public:
		virtual ~IService() {}

		// may throw!
		virtual void init() = 0;
		virtual void start() = 0;

		virtual void stop() = 0;

		virtual void close_server(const std::string& message) {}
		virtual void reopen_server() {}
	};
}
