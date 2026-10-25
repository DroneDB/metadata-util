// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <string>
#include <string_view>

namespace ddbexif::utf8 {

// Whether `text` is well-formed UTF-8 as JSON requires it: no overlong forms, no surrogates,
// nothing above U+10FFFF. An embedded NUL is valid.
bool isValid(std::string_view text);

// `text` with every ill-formed byte replaced by U+FFFD. For diagnostics only; the protocol
// carries data that must survive byte for byte as base64 instead.
std::string replaceInvalid(std::string_view text);

#ifdef _WIN32
// A UTF-16 string as UTF-8 (the command line arguments of wmain)
std::string fromWide(std::wstring_view wide);
#endif

}  // namespace ddbexif::utf8
