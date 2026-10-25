// SPDX-License-Identifier: GPL-2.0-or-later
#include "service.h"

#include <exception>
#include <set>

#include "exiv2_image.h"
#include "json_writer.h"
#include "tag_serializer.h"

namespace metadatautil {

namespace {

// A failure of a request that was understood
struct Failure {
    ErrorCode code;
    std::string message;
};

Failure describe(const std::exception_ptr& error) {
    try {
        std::rethrow_exception(error);
    } catch (const OpenError& e) {
        return {ErrorCode::Open, e.what()};
    } catch (const Exiv2::Error& e) {
        return {isOpenFailure(e) ? ErrorCode::Open : ErrorCode::Exiv2, e.what()};
    } catch (const std::exception& e) {
        // Anything else went wrong while reading the file (out of memory, an I/O error): the
        // request was fine, the file could not be served
        return {ErrorCode::Exiv2, e.what()};
    } catch (...) {
        return {ErrorCode::Exiv2, "unknown error"};
    }
}

}  // namespace

Response MetadataService::respondToLine(std::string_view line) const {
    try {
        return respond(parseRequest(line));
    } catch (const RequestError& e) {
        return {failureResponse(e.id(), ErrorCode::Request, e.what()), false};
    }
}

Response MetadataService::respond(const Request& request) const {
    try {
        return {request.op == Operation::Read ? readImage(request) : writeImage(request), true};
    } catch (...) {
        const Failure failure = describe(std::current_exception());
        return {failureResponse(request.id, failure.code, failure.message), false};
    }
}

Response MetadataService::read(int64_t id, const std::string& path) const {
    Request request;
    request.id = id;
    request.op = Operation::Read;
    request.path = path;
    return respond(request);
}

std::string MetadataService::readImage(const Request& request) const {
    auto image = openImage(request.path);
    image->readMetadata();

    JsonWriter writer;
    beginResponse(writer, request.id, true);
    writer.key("width").unsignedInteger(image->pixelWidth());
    writer.key("height").unsignedInteger(image->pixelHeight());
    writer.key("exif").beginArray();
    for (const auto& datum : image->exifData())
        writeTag(writer, datum, TagDetail::Full);
    writer.endArray();
    writer.key("xmp").beginArray();
    for (const auto& datum : image->xmpData())
        writeTag(writer, datum, TagDetail::Full);
    writer.endArray();
    writer.endObject();
    return writer.release();
}

std::string MetadataService::writeImage(const Request& request) const {
    auto image = openImage(request.path);
    image->readMetadata();

    Exiv2::ExifData& exifData = image->exifData();
    std::vector<std::string> keys;
    std::set<std::string> seen;
    for (const auto& assignment : request.set) {
        exifData[assignment.key] = assignment.value;
        if (seen.insert(assignment.key).second)
            keys.push_back(assignment.key);
    }
    saveImage(*image, request.path);

    JsonWriter writer;
    beginResponse(writer, request.id, true);
    writer.key("tags").beginArray();
    for (const auto& key : keys)
        writeTag(writer, exifData[key], TagDetail::Summary);
    writer.endArray();
    writer.endObject();
    return writer.release();
}

}  // namespace metadatautil
