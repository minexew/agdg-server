#pragma once

#include <string>

namespace agdg {
	class IService;

	class IServerLifecycle {
	public:
		virtual void Run() = 0;
		virtual void Start() = 0;

		virtual void request_shutdown() = 0;
		virtual void stop() = 0;

		virtual void close_server(const std::string& message) = 0;
		virtual void reopen_server() = 0;

		virtual IService* find_service_by_name(const std::string& name) = 0;
	};

	extern IServerLifecycle* g_serverLifecycle;
}
