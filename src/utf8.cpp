// SPDX-License-Identifier: GPL-2.0-or-later
#include "utf8.h"

#include <cstddef>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

namespace metadatautil::utf8 {

namespace {

bool isContinuation(unsigned char c) {
    return (c & 0xC0) == 0x80;
}

// The length of the well-formed sequence that starts at text[pos], 0 when there is none
// (Unicode 15, table 3-7).
size_t sequenceLength(std::string_view text, size_t pos) {
    const auto byte = [&](size_t i) { return static_cast<unsigned char>(text[i]); };
    const size_t left = text.size() - pos;
    const unsigned char b0 = byte(pos);

    if (b0 < 0x80)
        return 1;
    if (b0 >= 0xC2 && b0 <= 0xDF)
        return left >= 2 && isContinuation(byte(pos + 1)) ? 2 : 0;
    if (b0 >= 0xE0 && b0 <= 0xEF) {
        if (left < 3 || !isContinuation(byte(pos + 1)) || !isContinuation(byte(pos + 2)))
            return 0;
        const unsigned char b1 = byte(pos + 1);
        if (b0 == 0xE0 && b1 < 0xA0)  // overlong
            return 0;
        if (b0 == 0xED && b1 > 0x9F)  // surrogates
            return 0;
        return 3;
    }
    if (b0 >= 0xF0 && b0 <= 0xF4) {
        if (left < 4 || !isContinuation(byte(pos + 1)) || !isContinuation(byte(pos + 2)) ||
            !isContinuation(byte(pos + 3)))
            return 0;
        const unsigned char b1 = byte(pos + 1);
        if (b0 == 0xF0 && b1 < 0x90)  // overlong
            return 0;
        if (b0 == 0xF4 && b1 > 0x8F)  // above U+10FFFF
            return 0;
        return 4;
    }
    return 0;
}

}  // namespace

bool isValid(std::string_view text) {
    size_t pos = 0;
    while (pos < text.size()) {
        // Plain ASCII is the common case
        if (static_cast<unsigned char>(text[pos]) < 0x80) {
            ++pos;
            continue;
        }
        const size_t len = sequenceLength(text, pos);
        if (len == 0)
            return false;
        pos += len;
    }
    return true;
}

std::string replaceInvalid(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    size_t pos = 0;
    while (pos < text.size()) {
        const size_t len = sequenceLength(text, pos);
        if (len == 0) {
            out += "\xEF\xBF\xBD";
            ++pos;
        } else {
            out.append(text.substr(pos, len));
            pos += len;
        }
    }
    return out;
}

#ifdef _WIN32
std::string fromWide(std::wstring_view wide) {
    if (wide.empty())
        return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), out.data(), size, nullptr, nullptr);
    return out;
}
#endif

}  // namespace metadatautil::utf8
