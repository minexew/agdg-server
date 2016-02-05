#pragma once

#include <agdg/service.hpp>
#include <agdg/types.hpp>

#include <rapidjson/document.h>

namespace agdg {
	class ILoginServer : public IService {
	public:
		static unique_ptr<ILoginServer> Create(const std::string& serviceName, const rapidjson::Value& config);
	};
}
