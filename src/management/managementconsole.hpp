#pragma once

#include <agdg/service.hpp>
#include <agdg/types.hpp>

#include <rapidjson/document.h>

namespace agdg {
	class IManagementConsole : public IService {
	public:
		static unique_ptr<IManagementConsole> create(const std::string& serviceName, const rapidjson::Value& config);
	};
}
