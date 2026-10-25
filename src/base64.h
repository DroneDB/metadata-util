// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <string>
#include <string_view>

namespace metadatautil::base64 {

// Standard alphabet (RFC 4648 section 4), with padding
std::string encode(std::string_view bytes);

// False when `text` is not canonical-length standard base64 with correct padding
bool decode(std::string_view text, std::string& bytes);

}  // namespace metadatautil::base64
