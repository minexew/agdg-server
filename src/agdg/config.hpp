#pragma once

#include <rapidjson/document.h>

#include <functional>

namespace agdg {
	class IConfig {
	public:
		virtual void Init() = 0;

		virtual void EnumerateServices(std::function<void(const std::string&, const rapidjson::Value&)> callback) = 0;
	};

	extern IConfig* g_config;
}
