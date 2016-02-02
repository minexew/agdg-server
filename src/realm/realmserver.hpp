#pragma once

#include <agdg/service.hpp>

#include <rapidjson/document.h>

namespace agdg {
	class IRealmServer : public IService {
	public:
		static IRealmServer* Create(const std::string& serviceName, const rapidjson::Value& config);
	};
}
