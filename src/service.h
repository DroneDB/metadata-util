// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <string>
#include <string_view>

#include "protocol.h"

namespace metadatautil {

// One response line, without the line feed
struct Response {
    std::string line;
    bool ok = false;
};

// Answers requests. Every outcome becomes a response: a request that cannot be parsed or served
// never throws. Holds no state between requests.
class MetadataService {
public:
    // A request line as received: any text
    Response respondToLine(std::string_view line) const;

    Response respond(const Request& request) const;

    // The read response for `path`, as `dump` prints it
    Response read(int64_t id, const std::string& path) const;

private:
    std::string readImage(const Request& request) const;
    std::string writeImage(const Request& request) const;
};

}  // namespace metadatautil
