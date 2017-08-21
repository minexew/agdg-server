#pragma once

#include <reflection/basic_types.hpp>
#include <reflection/class.hpp>
#include <reflection/config.hpp>

#include <rapidjson/document.h>

namespace agdg {
    class RapidJsonConfigManager : public reflection::IConfigManager {
    public:
        RapidJsonConfigManager(const rapidjson::Value& d) : d(d) {}

        bool getValueForKey(reflection::IErrorHandler* err, const char* className, const char* fieldName, const char*& value_out) override {
            const auto& it = d.FindMember(fieldName);

            if (it == d.MemberEnd()) {
                value_out = nullptr;
                return true;
            }

            const auto& value = it->value;

            if (!value.IsString()) {
                err->errorf("ConfigError", "config value must be a string (key: '%s')", fieldName);
                return false;
            }

            buffer = value.GetString();
            value_out = buffer.c_str();
            return true;
        }

    private:
        const rapidjson::Value& d;

        std::string buffer;
    };

    template <typename T>
    void configure(T& inst, const rapidjson::Value& d) {
        // FIXME: need to check pre-condition d.IsObject()

        RapidJsonConfigManager configManager(d);

        if (!reflection::configure(inst, &configManager))
            // FIXME!!!
            throw std::exception();
    }

    template <typename T>
    void configureArray(std::vector<T>& list, const rapidjson::Value& d, const char* name) {
        // FIXME: need to check pre-condition d.IsObject()

        list.clear();

        const auto& it = d.FindMember(name);

        if (it == d.MemberEnd() || !it->value.IsArray())
            return;

        for (auto it2 = it->value.Begin(); it2 != it->value.End(); it2++) {
            T inst;
            configure(inst, *it2);
            list.push_back(std::move(inst));
        }
    }
}
