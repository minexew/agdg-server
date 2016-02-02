#pragma once

#include <agdg/service.hpp>

#include <rapidjson/document.h>

namespace agdg {
	class IManagementConsole : public IService {
	public:
		static IManagementConsole* Create(const std::string& serviceName, const rapidjson::Value& config);
	};
}
