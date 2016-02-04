#pragma once

#include <agdg/service.hpp>

#include <rapidjson/document.h>

namespace agdg {
	class ILoginServer : public IService {
	public:
		static ILoginServer* Create(const std::string& serviceName, const rapidjson::Value& config);
	};
}