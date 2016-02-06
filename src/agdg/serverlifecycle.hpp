#pragma once

#include <string>

namespace agdg {
	class IServerLifecycle {
	public:
		virtual void Run() = 0;
		virtual void Start() = 0;
		virtual void Stop() = 0;

		virtual void close_server(const std::string& message) = 0;
		virtual void reopen_server() = 0;
	};

	extern IServerLifecycle* g_serverLifecycle;
}
