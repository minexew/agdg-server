#pragma once

#include <cstdint>
#include <functional>

namespace agdg {
	struct SHA3_224 {
		uint8_t bytes[28];

		bool operator==(const SHA3_224& other) const {
			return memcmp(bytes, other.bytes, sizeof(bytes)) == 0;
		}
	};

	struct SHA3_512 {
		uint8_t bytes[64];
	};
}

namespace std {
	template <>
	struct hash<agdg::SHA3_224> {
		size_t operator()(const agdg::SHA3_224& hash) const {
			return *reinterpret_cast<const size_t*>(&hash.bytes[0]);
		}
	};
}
