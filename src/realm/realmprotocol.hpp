#pragma once

#include <agdg/types.hpp>

#include <string>
#include <vector>

namespace agdg {
#include "protocol_generated.hpp"

	bool read_command_header(const uint8_t*& buffer, size_t& length, uint8_t& code_out,
			uint8_t& cookie_out, uint16_t& payload_length_out);
}
