// SPDX-License-Identifier: GPL-2.0-or-later
#include "protocol.h"

#include <nlohmann/json.hpp>

#include "base64.h"
#include "utf8.h"

namespace ddbexif {

namespace {

using json = nlohmann::json;

// The string member `name`, or `name`64 decoded; nullopt when neither is present.
// Throws RequestError when both are, when the member has another type or the base64 is wrong.
std::optional<std::string> readTextField(const json& object, const std::string& name, std::optional<int64_t> id) {
    const auto plain = object.find(name);
    const auto encoded = object.find(name + "64");
    if (plain != object.end() && encoded != object.end())
        throw RequestError(id, "\"" + name + "\" and \"" + name + "64\" are both present");

    if (plain != object.end()) {
        if (!plain->is_string())
            throw RequestError(id, "\"" + name + "\" must be a string");
        return plain->get<std::string>();
    }
    if (encoded != object.end()) {
        if (!encoded->is_string())
            throw RequestError(id, "\"" + name + "64\" must be a string");
        std::string bytes;
        if (!base64::decode(encoded->get_ref<const std::string&>(), bytes))
            throw RequestError(id, "\"" + name + "64\" is not valid base64");
        return bytes;
    }
    return std::nullopt;
}

std::optional<int64_t> readId(const json& object) {
    const auto it = object.find("id");
    if (it == object.end() || !it->is_number_integer())
        return std::nullopt;
    if (it->is_number_unsigned() && it->get<uint64_t>() > static_cast<uint64_t>(INT64_MAX))
        return std::nullopt;
    return it->get<int64_t>();
}

std::vector<TagAssignment> readAssignments(const json& object, std::optional<int64_t> id) {
    const auto it = object.find("set");
    if (it == object.end() || !it->is_array())
        throw RequestError(id, "\"set\" must be an array");

    std::vector<TagAssignment> assignments;
    assignments.reserve(it->size());
    for (const auto& item : *it) {
        if (!item.is_object())
            throw RequestError(id, "every member of \"set\" must be an object");
        auto key = item.find("key");
        if (key == item.end() || !key->is_string() || key->get_ref<const std::string&>().empty())
            throw RequestError(id, "every member of \"set\" needs a non-empty \"key\"");
        auto value = readTextField(item, "value", id);
        if (!value)
            throw RequestError(id, "every member of \"set\" needs a \"value\" or a \"value64\"");
        assignments.push_back({key->get<std::string>(), std::move(*value)});
    }
    return assignments;
}

}  // namespace

const char* errorName(ErrorCode code) {
    switch (code) {
        case ErrorCode::Exiv2: return "exiv2";
        case ErrorCode::Open: return "open";
        case ErrorCode::Request: return "request";
    }
    return "request";
}

Request parseRequest(std::string_view line) {
    json document;
    try {
        document = json::parse(line.begin(), line.end());
    } catch (const json::exception& e) {
        throw RequestError(std::nullopt, std::string("not a valid JSON line: ") + e.what());
    }
    if (!document.is_object())
        throw RequestError(std::nullopt, "a request must be a JSON object");

    const std::optional<int64_t> id = readId(document);
    if (!id)
        throw RequestError(std::nullopt, "\"id\" must be an integer");

    const auto op = document.find("op");
    if (op == document.end() || !op->is_string())
        throw RequestError(id, "\"op\" must be a string");

    Request request;
    request.id = *id;
    const std::string& opName = op->get_ref<const std::string&>();
    if (opName == "read")
        request.op = Operation::Read;
    else if (opName == "write")
        request.op = Operation::Write;
    else
        throw RequestError(id, "unknown op \"" + utf8::replaceInvalid(opName.substr(0, 64)) + "\"");

    auto path = readTextField(document, "path", id);
    if (!path || path->empty())
        throw RequestError(id, "a request needs a non-empty \"path\" or \"path64\"");
    if (path->find('\0') != std::string::npos)
        throw RequestError(id, "\"path\" holds a NUL character");
    request.path = std::move(*path);

    if (request.op == Operation::Write)
        request.set = readAssignments(document, id);
    return request;
}

void writeTextField(JsonWriter& writer, std::string_view name, std::string_view bytes) {
    if (utf8::isValid(bytes)) {
        writer.key(name).string(bytes);
    } else {
        writer.key(std::string(name) + "64").string(base64::encode(bytes));
    }
}

void beginResponse(JsonWriter& writer, std::optional<int64_t> id, bool ok) {
    writer.beginObject().key("id");
    if (id)
        writer.integer(*id);
    else
        writer.null();
    writer.key("ok").boolean(ok);
}

std::string failureResponse(std::optional<int64_t> id, ErrorCode code, std::string_view message) {
    JsonWriter writer;
    beginResponse(writer, id, false);
    writer.key("error").string(errorName(code));
    writer.key("message").string(utf8::replaceInvalid(message.substr(0, limits::MaxMessageBytes)));
    writer.endObject();
    return writer.release();
}

}  // namespace ddbexif
