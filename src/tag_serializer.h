// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <exiv2/exiv2.hpp>

#include "json_writer.h"

// Turns Exiv2 metadata into the tag objects of PROTOCOL.md.
namespace ddbexif {

enum class TagDetail {
    // k, t, n, s and the component values v
    Full,
    // k, t, n, s
    Summary,
};

void writeTag(JsonWriter& writer, const Exiv2::Exifdatum& datum, TagDetail detail);
void writeTag(JsonWriter& writer, const Exiv2::Xmpdatum& datum, TagDetail detail);

}  // namespace ddbexif
