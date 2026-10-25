// SPDX-License-Identifier: GPL-2.0-or-later
#include "base64.h"

#include <array>
#include <cstdint>

namespace ddbexif::base64 {

namespace {

constexpr char Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Value of each alphabet character, -1 for any other byte
constexpr std::array<int8_t, 256> makeDecodeTable() {
    std::array<int8_t, 256> table{};
    for (auto& v : table)
        v = -1;
    for (int i = 0; i < 64; ++i)
        table[static_cast<unsigned char>(Alphabet[i])] = static_cast<int8_t>(i);
    return table;
}

constexpr std::array<int8_t, 256> DecodeTable = makeDecodeTable();

}  // namespace

std::string encode(std::string_view bytes) {
    std::string out;
    out.reserve((bytes.size() + 2) / 3 * 4);
    size_t i = 0;
    for (; i + 3 <= bytes.size(); i += 3) {
        const uint32_t n = (static_cast<uint8_t>(bytes[i]) << 16) | (static_cast<uint8_t>(bytes[i + 1]) << 8) |
                           static_cast<uint8_t>(bytes[i + 2]);
        out += Alphabet[(n >> 18) & 63];
        out += Alphabet[(n >> 12) & 63];
        out += Alphabet[(n >> 6) & 63];
        out += Alphabet[n & 63];
    }
    const size_t rest = bytes.size() - i;
    if (rest == 1) {
        const uint32_t n = static_cast<uint8_t>(bytes[i]) << 16;
        out += Alphabet[(n >> 18) & 63];
        out += Alphabet[(n >> 12) & 63];
        out += "==";
    } else if (rest == 2) {
        const uint32_t n = (static_cast<uint8_t>(bytes[i]) << 16) | (static_cast<uint8_t>(bytes[i + 1]) << 8);
        out += Alphabet[(n >> 18) & 63];
        out += Alphabet[(n >> 12) & 63];
        out += Alphabet[(n >> 6) & 63];
        out += '=';
    }
    return out;
}

bool decode(std::string_view text, std::string& bytes) {
    bytes.clear();
    if (text.size() % 4 != 0)
        return false;
    bytes.reserve(text.size() / 4 * 3);
    for (size_t i = 0; i < text.size(); i += 4) {
        const bool last = i + 4 == text.size();
        int padding = 0;
        uint32_t n = 0;
        for (size_t j = 0; j < 4; ++j) {
            const unsigned char c = static_cast<unsigned char>(text[i + j]);
            n <<= 6;
            if (c == '=') {
                // Padding only closes the last quantum, one or two characters of it
                if (!last || j < 2)
                    return false;
                ++padding;
            } else {
                const int v = DecodeTable[c];
                if (v < 0 || padding > 0)
                    return false;
                n |= static_cast<uint32_t>(v);
            }
        }
        bytes += static_cast<char>((n >> 16) & 0xFF);
        if (padding < 2)
            bytes += static_cast<char>((n >> 8) & 0xFF);
        if (padding < 1)
            bytes += static_cast<char>(n & 0xFF);
    }
    return true;
}

}  // namespace ddbexif::base64
