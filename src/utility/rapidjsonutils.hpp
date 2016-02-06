#pragma once

#include <rapidjson/document.h>

namespace agdg {
	using namespace std::literals::string_literals;

	class RapidJsonUtils {
	public:
		template <typename List>
		static void get_array(List& list, const rapidjson::Value& object, const char* field_name) {
			// FIXME: error formulation should go in here

			assert(object.IsObject());

			list.clear();

			const auto& it = object.FindMember(field_name);

			if (it == object.MemberEnd() || !it->value.IsArray())
				throw std::runtime_error("field '"s + field_name + "' not found");

			try {
				for (auto it2 = it->value.Begin(); it2 != it->value.End(); it2++) {
					list.emplace_back();
					get_value(list.back(), *it2);
				}
			}
			catch (std::runtime_error& ex) {
				throw std::runtime_error(ex.what() + " in "s + field_name);
			}
		}

		static void get_value(std::string& output, const rapidjson::Value& value) {
			if (!value.IsString())
				throw std::runtime_error("expected string");

			output.assign(value.GetString(), value.GetStringLength());
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
