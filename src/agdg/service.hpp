#pragma once

#include <string>

namespace agdg {
	class IService {
	public:
		virtual ~IService() {}

		virtual void Init() = 0;
		virtual void Start() = 0;
		virtual void Stop() = 0;

		virtual void close_server(const std::string& message) {}
		virtual void reopen_server() {}
	};
}
