#pragma once

#include <utility/chronoutils.hpp>
#include <utility/fileutils.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <ostream>
#include <stdexcept>
#include <string>

namespace agdg {
    using namespace std::string_literals;

    class RapidJsonUtils {
    public:
        template <typename List>
        static void get_array(List& list, const rapidjson::Value& object, const char* field_name) {
            list.clear();

            auto& array = get_member(object, field_name);

            if (!array.IsArray())
                throw std::runtime_error("expected array for '"s + field_name + "'d");

            try {
                for (auto it = array.Begin(); it != array.End(); it++) {
                    list.emplace_back();
                    get_value(list.back(), *it);
                }
            }
            catch (std::runtime_error& ex) {
                throw std::runtime_error(ex.what() + " in '"s + field_name + "'");
            }
        }

        template <typename T>
        static void get_value(T& output, const rapidjson::Value& object, const char* field_name) {
            auto& value = get_member(object, field_name);

            try {
                get_value(output, value);
            }
            catch (std::runtime_error& ex) {
                throw std::runtime_error(ex.what() + " for '"s + field_name + "'");
            }
        }

        static void get_value(std::string& output, const rapidjson::Value& value) {
            if (!value.IsString())
                throw std::runtime_error("expected string");

            output.assign(value.GetString(), value.GetStringLength());
        }

        static void get_value(std::chrono::system_clock::time_point& output, const rapidjson::Value& value) {
            if (!value.IsString())
                throw std::runtime_error("expected string");

            ChronoUtils::from_iso8601(output, std::string(value.GetString(), value.GetStringLength()));
        }

        static void load_json(rapidjson::Document& d_out, const char* path) {
            std::string json = FileUtils::get_contents(path);
            rapidjson::ParseResult ok = d_out.Parse(json.c_str());

            if (!ok)
                throw std::runtime_error(path + ": JSON parse error: "s + rapidjson::GetParseError_En(ok.Code()) + " (" + std::to_string(ok.Offset()) + ")");
        }

        static bool try_load_json(rapidjson::Document& d_out, const char* path) {
            std::string json;

            if (!FileUtils::try_get_contents(path, json))
                return false;

            rapidjson::ParseResult ok = d_out.Parse(json.c_str());

            if (!ok)
                throw std::runtime_error(path + ": JSON parse error: "s + rapidjson::GetParseError_En(ok.Code()) + " (" + std::to_string(ok.Offset()) + ")");

            return true;
        }

    private:
        static const rapidjson::Value& get_member(const rapidjson::Value& object, const char* field_name) {
            assert(object.IsObject());
            const auto& it = object.FindMember(field_name);

            if (it == object.MemberEnd())
                throw std::runtime_error("field '"s + field_name + "' not found");

            return it->value;
        }
    };

    class RapidJsonOstream {
    public:
        typedef char Ch;

        RapidJsonOstream(std::ostream& os) : os(os) {}

        void Flush() {
            os.flush();
        }

        void Put(char c) { 
            os.put(c);
        }

    private:
        std::ostream& os;
    };

    template <typename T>
    bool getBool(const T& d, const char* name, bool& value_out) {
        const auto& it = d.FindMember(name);

        if (it == d.MemberEnd() || !it->value.IsBool())
            return false;

        value_out = it->value.GetBool();
        return true;
    }

    template <typename T>
    bool getInt(const T& d, const char* name, int& value_out) {
        const auto& it = d.FindMember(name);

        if (it == d.MemberEnd() || !it->value.IsInt())
            return false;

        value_out = it->value.GetInt();
        return true;
    }

    template <typename T>
    bool getString(const T& d, const char* name, std::string& value_out) {
        const auto& it = d.FindMember(name);

        if (it == d.MemberEnd() || !it->value.IsString())
            return false;

        value_out = it->value.GetString();
        return true;
    }
}
