#pragma once

#include <agdg/types.hpp>

#include <rhash/sha3.h>

#include <string>

namespace agdg {
	class HashUtils {
	public:
		static void HashString(const std::string& str, SHA3_224& hash_out) {
			sha3_ctx ctx;

			rhash_sha3_224_init(&ctx);
			rhash_sha3_update(&ctx, (const uint8_t*)str.c_str(), str.size());
			rhash_sha3_final(&ctx, hash_out.bytes);
		}

		static void HashString(const std::string& str, SHA3_512& hash_out) {
			sha3_ctx ctx;

			rhash_sha3_512_init(&ctx);
			rhash_sha3_update(&ctx, (const uint8_t*)str.c_str(), str.size());
			rhash_sha3_final(&ctx, hash_out.bytes);
		}

		template <typename T>
		static std::string HashToHexString(const T& hash) {
			char string[sizeof(hash.bytes) * 2 + 1];

			for (int i = 0; i < sizeof(hash.bytes); i++)
				snprintf(string + i * 2, 3, "%02x", hash.bytes[i]);

			return string;
		}

		static std::string StringToSHA3_512Hex(const std::string& str) {
			SHA3_512 hash;
			HashString(str, hash);
			return HashToHexString(hash);
		}
	};
}