// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace metadatautil {

// Builds one JSON document in insertion order. Responses are written by hand instead of through
// a JSON value tree: a read response holds thousands of numbers, and member order and float
// text (the shortest text that reads back as the same float) are part of the protocol.
//
// Strings must be well-formed UTF-8; the caller checks (see protocol.h, writeTextField).
class JsonWriter {
public:
    JsonWriter& beginObject();
    JsonWriter& endObject();
    JsonWriter& beginArray();
    JsonWriter& endArray();

    // Starts the member `name` of the open object; its value follows
    JsonWriter& key(std::string_view name);

    JsonWriter& null();
    JsonWriter& boolean(bool value);
    JsonWriter& integer(int64_t value);
    JsonWriter& unsignedInteger(uint64_t value);
    // A finite float as its shortest round-trip text; NaN, Infinity and -Infinity as JSON strings
    JsonWriter& real(float value);
    JsonWriter& string(std::string_view utf8);

    const std::string& text() const { return out_; }
    std::string release() { return std::move(out_); }

private:
    void beforeValue();
    void appendQuoted(std::string_view utf8);

    std::string out_;
    // For each open container: whether it already holds an element
    std::vector<bool> hasElement_;
    bool afterKey_ = false;
};

}  // namespace metadatautil
