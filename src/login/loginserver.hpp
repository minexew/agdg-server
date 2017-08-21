#pragma once

#include <agdg/service.hpp>
#include <agdg/types.hpp>

#include <rapidjson/document.h>

namespace agdg {
    class ILoginServer : public IService {
    public:
        static unique_ptr<ILoginServer> create(const std::string& serviceName, const rapidjson::Value& config);

        virtual void post_news(std::string&& title_html, std::string&& contents_html) = 0;
    };
}
