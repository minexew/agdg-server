#include <realm/realmprotocol.hpp>

#include <iostream>

namespace agdg {
	inline bool read(const uint8_t*& buffer, size_t& length, void* output, size_t count) {
		if (length < count)
			return false;

		memcpy(output, buffer, count);
		buffer += count;
		length -= count;
		return true;
	}

	// TODO: this doesn't seem to get inlined at -O0
	// investigate if this is an issue
	inline bool write(std::vector<uint8_t>& out, const uint8_t* bytes, size_t count) {
		out.resize(out.size() + count);
		memcpy(&out[out.size() - count], bytes, count);
		return true;
	}

	template <typename T>
	bool read(const uint8_t*& buffer, size_t& length, T& value_out);

	template <typename T>
	bool write(std::vector<uint8_t>& out, const T& value);

	template <>
	bool read(const uint8_t*& buffer, size_t& length, int32_t& value_out) {
		return read(buffer, length, &value_out, 4);
	}

	template <>
	bool read(const uint8_t*& buffer, size_t& length, uint8_t& value_out) {
		return read(buffer, length, &value_out, 1);
	}

	template <>
	bool read(const uint8_t*& buffer, size_t& length, uint16_t& value_out) {
		return read(buffer, length, &value_out, 2);
	}

	template <>
	bool read(const uint8_t*& buffer, size_t& length, uint32_t& value_out) {
		return read(buffer, length, &value_out, 4);
	}

	template <>
	bool read(const uint8_t*& buffer, size_t& length, bool& value_out) {
		uint8_t u8;

		if (read(buffer, length, &u8, 1)) {
			value_out = (u8 != 0);
			return true;
		}
		else
			return false;
	}

	template <>
	bool read(const uint8_t*& buffer, size_t& length, SHA3_224& value_out) {
		return read(buffer, length, &value_out.bytes, sizeof(value_out.bytes));
	}

	template <>
	bool read(const uint8_t*& buffer, size_t& length, glm::vec3& value_out) {
		return read(buffer, length, &value_out.xyz[0], 12);
	}

	template <>
	bool read(const uint8_t*& buffer, size_t& length, std::string& value_out) {
		uint32_t len;
		if (!read(buffer, length, len))
			return false;

		value_out.resize(len);
		return read(buffer, length, &value_out[0], len);
	}

	template <>
	bool write<>(std::vector<uint8_t>& out, const int32_t& value) {
		write(out, reinterpret_cast<const uint8_t*>(&value), 4);
		return true;
	}

	template <>
	bool write<>(std::vector<uint8_t>& out, const uint8_t& value) {
		write(out, reinterpret_cast<const uint8_t*>(&value), 1);
		return true;
	}

	template <>
	bool write<>(std::vector<uint8_t>& out, const uint16_t& value) {
		write(out, reinterpret_cast<const uint8_t*>(&value), 2);
		return true;
	}

	template <>
	bool write<>(std::vector<uint8_t>& out, const uint32_t& value) {
		write(out, reinterpret_cast<const uint8_t*>(&value), 4);
		return true;
	}

	template <>
	bool write<>(std::vector<uint8_t>& out, const bool& value) {
		return write<uint8_t>(out, value ? 1 : 0);
	}

	template <>
	bool write<>(std::vector<uint8_t>& out, const SHA3_224& value) {
		write(out, reinterpret_cast<const uint8_t*>(&value.bytes), sizeof(value.bytes));
		return true;
	}

	template <>
	bool write<>(std::vector<uint8_t>& out, const glm::vec3& value) {
		write(out, reinterpret_cast<const uint8_t*>(&value), 12);
		return true;
	}

	template <>
	bool write<>(std::vector<uint8_t>& out, const std::string& value) {
		if (value.size() > UINT32_MAX)
			return false;

		write<uint32_t>(out, (uint32_t) value.size());
		write(out, reinterpret_cast<const uint8_t*>(value.c_str()), value.size());
		return true;
	}

	bool read_command_header(const uint8_t*& buffer, size_t& length, uint8_t& code_out,
			uint8_t& cookie_out, uint16_t& payload_length_out) {
		if (length < 4)
			return false;

		return read(buffer, length, code_out)
				&& read(buffer, length, cookie_out)
				&& read(buffer, length, payload_length_out);
	}

	static size_t begin(std::vector<uint8_t>& out, uint8_t code, uint8_t cookie) {
		if (!(write<uint8_t>(out, code)
				&& write<uint8_t>(out, cookie)
				&& write<uint16_t>(out, 0)))
			return false;

		return out.size() - 2;
	}

	static bool end(std::vector<uint8_t>& out, size_t original_offset) {
		const uint16_t payload_length = out.size() - (original_offset + 2);
		memcpy(&out[original_offset], &payload_length, 2);
		return true;
	}

#include "protocol_generated_impl.hpp"
}
