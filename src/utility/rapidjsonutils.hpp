#pragma once

#include <rapidjson/document.h>

#include <string>

namespace agdg {
	using namespace std::literals::string_literals;

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

	private:
		static const rapidjson::Value& get_member(const rapidjson::Value& object, const char* field_name) {
			assert(object.IsObject());
			const auto& it = object.FindMember(field_name);

			if (it == object.MemberEnd())
				throw std::runtime_error("field '"s + field_name + "' not found");

			return it->value;
		}
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
