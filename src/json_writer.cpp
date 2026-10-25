// SPDX-License-Identifier: GPL-2.0-or-later
#include "json_writer.h"

#include <charconv>
#include <cmath>

namespace metadatautil {

void JsonWriter::beforeValue() {
    if (afterKey_) {
        afterKey_ = false;
        return;
    }
    if (hasElement_.empty())
        return;
    if (hasElement_.back())
        out_ += ',';
    hasElement_.back() = true;
}

JsonWriter& JsonWriter::beginObject() {
    beforeValue();
    out_ += '{';
    hasElement_.push_back(false);
    return *this;
}

JsonWriter& JsonWriter::endObject() {
    out_ += '}';
    hasElement_.pop_back();
    return *this;
}

JsonWriter& JsonWriter::beginArray() {
    beforeValue();
    out_ += '[';
    hasElement_.push_back(false);
    return *this;
}

JsonWriter& JsonWriter::endArray() {
    out_ += ']';
    hasElement_.pop_back();
    return *this;
}

JsonWriter& JsonWriter::key(std::string_view name) {
    beforeValue();
    appendQuoted(name);
    out_ += ':';
    afterKey_ = true;
    return *this;
}

JsonWriter& JsonWriter::null() {
    beforeValue();
    out_ += "null";
    return *this;
}

JsonWriter& JsonWriter::boolean(bool value) {
    beforeValue();
    out_ += value ? "true" : "false";
    return *this;
}

JsonWriter& JsonWriter::integer(int64_t value) {
    beforeValue();
    char buf[24];
    const auto result = std::to_chars(buf, buf + sizeof buf, value);
    out_.append(buf, result.ptr);
    return *this;
}

JsonWriter& JsonWriter::unsignedInteger(uint64_t value) {
    beforeValue();
    char buf[24];
    const auto result = std::to_chars(buf, buf + sizeof buf, value);
    out_.append(buf, result.ptr);
    return *this;
}

namespace {

// The shortest text that reads back as `value` once a client parses it as a double and converts
// that to float, which is what JSON libraries hand out. The shortest float text does that for all
// but a few floats (7.038531e-26 is one: as a double it rounds to a value on the wrong side of
// a float midpoint); those get the exact value of the float as a double.
std::string floatText(float value) {
    char buf[48];

    // Whole numbers below 2^24 are exact in a float and in a double: byte values and small
    // counts, nearly everything a read response holds. -0 keeps its sign through the general path.
    if (value == std::trunc(value) && std::fabs(value) < 16777216.0F && !(value == 0 && std::signbit(value))) {
        const auto whole = std::to_chars(buf, buf + sizeof buf, static_cast<int32_t>(value));
        return std::string(buf, whole.ptr);
    }

    auto result = std::to_chars(buf, buf + sizeof buf, value);

    double parsed = 0;
    const auto back = std::from_chars(buf, result.ptr, parsed);
    if (back.ec != std::errc() || static_cast<float>(parsed) != value)
        result = std::to_chars(buf, buf + sizeof buf, static_cast<double>(value));
    return std::string(buf, result.ptr);
}

}  // namespace

JsonWriter& JsonWriter::real(float value) {
    if (std::isnan(value))
        return string("NaN");
    if (std::isinf(value))
        return string(value > 0 ? "Infinity" : "-Infinity");

    beforeValue();
    const std::string text = floatText(value);
    out_ += text;
    // Keep the member a JSON float: 5 becomes 5.0
    if (text.find_first_of(".eE") == std::string::npos)
        out_ += ".0";
    return *this;
}

JsonWriter& JsonWriter::string(std::string_view utf8) {
    beforeValue();
    appendQuoted(utf8);
    return *this;
}

void JsonWriter::appendQuoted(std::string_view utf8) {
    out_ += '"';
    for (const char ch : utf8) {
        const unsigned char c = static_cast<unsigned char>(ch);
        switch (c) {
            case '"': out_ += "\\\""; break;
            case '\\': out_ += "\\\\"; break;
            case '\n': out_ += "\\n"; break;
            case '\r': out_ += "\\r"; break;
            case '\t': out_ += "\\t"; break;
            case '\b': out_ += "\\b"; break;
            case '\f': out_ += "\\f"; break;
            default:
                if (c < 0x20) {
                    static constexpr char Hex[] = "0123456789abcdef";
                    out_ += "\\u00";
                    out_ += Hex[c >> 4];
                    out_ += Hex[c & 15];
                } else {
                    out_ += ch;
                }
        }
    }
    out_ += '"';
}

}  // namespace metadatautil
