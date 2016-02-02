#pragma once

#include <rapidjson/document.h>

namespace agdg {
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
