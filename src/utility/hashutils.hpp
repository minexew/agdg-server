#pragma once

#include <agdg/types.hpp>

#include <rhash/sha3.h>

#include <string>

namespace agdg {
	class HashUtils {
	public:
		static void hash_bytes(const uint8_t* bytes, size_t length, SHA3_224& hash_out) {
			sha3_ctx ctx;

			rhash_sha3_224_init(&ctx);
			rhash_sha3_update(&ctx, bytes, length);
			rhash_sha3_final(&ctx, hash_out.bytes);
		}

		static void hash_bytes(const uint8_t* bytes, size_t length, SHA3_512& hash_out) {
			sha3_ctx ctx;

			rhash_sha3_512_init(&ctx);
			rhash_sha3_update(&ctx, bytes, length);
			rhash_sha3_final(&ctx, hash_out.bytes);
		}

		template <typename T>
		static void hash_string(const std::string& str, T& hash_out) {
			hash_bytes((const uint8_t*)str.c_str(), str.size(), hash_out);
		}

		template <typename T>
		static std::string hash_to_hex_string(const T& hash) {
			char string[sizeof(hash.bytes) * 2 + 1];

			for (size_t i = 0; i < sizeof(hash.bytes); i++)
				snprintf(string + i * 2, 3, "%02x", hash.bytes[i]);

			return string;
		}

		static std::string string_to_sha3_512_hex_string(const std::string& str) {
			SHA3_512 hash;
			hash_string(str, hash);
			return hash_to_hex_string(hash);
		}
	};
}