#include <realm/realmprotocol.hpp>

#include <iostream>

namespace agdg {
	inline bool Read(const uint8_t*& buffer, size_t& length, void* output, size_t count) {
		if (length < count)
			return false;

		memcpy(output, buffer, count);
		buffer += count;
		length -= count;
		return true;
	}

	template <typename T>
	bool Read(const uint8_t*& buffer, size_t& length, T& value_out);

	template <typename T>
	bool Write(std::ostream& out, const T& value);

	template <>
	bool Read(const uint8_t*& buffer, size_t& length, int32_t& value_out) {
		return Read(buffer, length, &value_out, 4);
	}

	template <>
	bool Read(const uint8_t*& buffer, size_t& length, uint8_t& value_out) {
		return Read(buffer, length, &value_out, 1);
	}

	template <>
	bool Read(const uint8_t*& buffer, size_t& length, uint32_t& value_out) {
		return Read(buffer, length, &value_out, 4);
	}

	template <>
	bool Read(const uint8_t*& buffer, size_t& length, bool& value_out) {
		uint8_t u8;

		if (Read(buffer, length, &u8, 1)) {
			value_out = (u8 != 0);
			return true;
		}
		else
			return false;
	}

	template <>
	bool Read(const uint8_t*& buffer, size_t& length, SHA3_224& value_out) {
		return Read(buffer, length, &value_out.bytes, sizeof(value_out.bytes));
	}

	template <>
	bool Read(const uint8_t*& buffer, size_t& length, glm::vec3& value_out) {
		return Read(buffer, length, &value_out.xyz[0], 12);
	}

	template <>
	bool Read(const uint8_t*& buffer, size_t& length, std::string& value_out) {
		uint32_t len;
		if (!Read(buffer, length, len))
			return false;

		value_out.resize(len);
		return Read(buffer, length, &value_out[0], len);
	}

	template <>
	bool Write<>(std::ostream& out, const int32_t& value) {
		out.write(reinterpret_cast<const char*>(&value), 4);
		return true;
	}

	template <>
	bool Write<>(std::ostream& out, const uint8_t& value) {
		out.write(reinterpret_cast<const char*>(&value), 1);
		return true;
	}

	template <>
	bool Write<>(std::ostream& out, const uint32_t& value) {
		out.write(reinterpret_cast<const char*>(&value), 4);
		return true;
	}

	template <>
	bool Write<>(std::ostream& out, const bool& value) {
		return Write<uint8_t>(out, value ? 1 : 0);
	}

	template <>
	bool Write<>(std::ostream& out, const SHA3_224& value) {
		out.write(reinterpret_cast<const char*>(&value.bytes), sizeof(value.bytes));
		return true;
	}

	template <>
	bool Write<>(std::ostream& out, const glm::vec3& value) {
		out.write(reinterpret_cast<const char*>(&value), 12);
		return true;
	}

	template <>
	bool Write<>(std::ostream& out, const std::string& value) {
		if (value.size() > UINT32_MAX)
			return false;

		Write<uint32_t>(out, (uint32_t) value.size());
		out.write(value.c_str(), value.size());
		return true;
	}

	static bool Begin(std::ostream& out, int messageId) {
		return Write<uint8_t>(out, messageId);
	}

#include "protocol_generated_impl.hpp"
}
