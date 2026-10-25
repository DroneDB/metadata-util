// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "json_writer.h"

// The wire format of PROTOCOL.md: requests parsed from one line each, responses built as one line.
namespace ddbexif {

namespace limits {
// A tag whose toString() is longer is sent without its value (trunc)
constexpr size_t MaxTagStringBytes = 65536;
// Components of a tag that get an entry in "v"
constexpr size_t MaxComponentValues = 64;
// Entries "v" holds at least, tags with fewer components included: callers read the first three
// components of a GPS coordinate without checking the count
constexpr size_t MinComponentValues = 3;
// Longest request line served
constexpr size_t MaxRequestLineBytes = 16 * 1024 * 1024;
// Longest "message" of a failure response
constexpr size_t MaxMessageBytes = 2000;
}  // namespace limits

enum class ErrorCode { Exiv2, Open, Request };

// The "error" member of a failure response
const char* errorName(ErrorCode code);

enum class Operation { Read, Write };

struct TagAssignment {
    std::string key;
    std::string value;  // bytes, as decoded from "value" or "value64"
};

struct Request {
    int64_t id = 0;
    Operation op = Operation::Read;
    std::string path;                  // bytes, as decoded from "path" or "path64"
    std::vector<TagAssignment> set;    // write only
};

// A request line that cannot be served. `id` is set when the line was a JSON object with a
// usable id, so the response can carry it.
class RequestError : public std::runtime_error {
public:
    RequestError(std::optional<int64_t> id, const std::string& message) : std::runtime_error(message), id_(id) {}
    std::optional<int64_t> id() const { return id_; }

private:
    std::optional<int64_t> id_;
};

// Throws RequestError
Request parseRequest(std::string_view line);

// Writes the member `name` holding `bytes` when they are well-formed UTF-8, else the member
// `name`64 holding their base64: the bytes arrive unchanged either way.
void writeTextField(JsonWriter& writer, std::string_view name, std::string_view bytes);

// Starts a response object: {"id":<id>,"ok":<ok> (the caller adds members and closes it)
void beginResponse(JsonWriter& writer, std::optional<int64_t> id, bool ok);

// The complete failure response line (without the line feed)
std::string failureResponse(std::optional<int64_t> id, ErrorCode code, std::string_view message);

}  // namespace ddbexif
