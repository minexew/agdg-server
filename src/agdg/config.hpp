#pragma once

#include <agdg/types.hpp>
#include <rapidjson/document.h>

#include <functional>

namespace agdg {
    class IConfig {
    public:
        virtual void init(const char* master_config_or_null) = 0;

        virtual void enumerate_services(std::function<void(const std::string&, const rapidjson::Value&)> callback) = 0;

        virtual void get_value(std::string& output, const char* field_name) = 0;
    };

    extern IConfig* g_config;
}
