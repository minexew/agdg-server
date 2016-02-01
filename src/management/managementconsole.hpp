#pragma once

#include <agdg/service.hpp>

#include <rapidjson/document.h>

namespace agdg {
	class IManagementConsole : public IService {
	public:
		static IManagementConsole* Create(const rapidjson::Value& config);
	};
}
