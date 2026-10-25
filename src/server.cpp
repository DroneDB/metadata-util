// SPDX-License-Identifier: GPL-2.0-or-later
#include "server.h"

#include <istream>
#include <ostream>
#include <streambuf>
#include <string>

#include "protocol.h"

namespace metadatautil {

namespace {

enum class LineStatus { Line, TooLong, End };

// Reads up to the next line feed. A line over the limit is consumed to its end and not kept, so
// the next request starts on the next line. The last line may lack the line feed.
LineStatus readLine(std::streambuf& in, std::string& line) {
    line.clear();
    bool tooLong = false;
    bool any = false;
    for (int c = in.sbumpc(); c != std::streambuf::traits_type::eof(); c = in.sbumpc()) {
        any = true;
        if (c == '\n')
            return tooLong ? LineStatus::TooLong : LineStatus::Line;
        if (line.size() < limits::MaxRequestLineBytes)
            line.push_back(static_cast<char>(c));
        else
            tooLong = true;
    }
    if (!any)
        return LineStatus::End;
    return tooLong ? LineStatus::TooLong : LineStatus::Line;
}

}  // namespace

int serve(std::istream& in, std::ostream& out, const MetadataService& service) {
    std::string line;
    for (;;) {
        const LineStatus status = readLine(*in.rdbuf(), line);
        if (status == LineStatus::End)
            return 0;

        const std::string response =
            status == LineStatus::TooLong
                ? failureResponse(std::nullopt, ErrorCode::Request, "the request line is longer than 16 MiB")
                : service.respondToLine(line).line;
        out << response << '\n' << std::flush;
        if (!out)
            return 1;
    }
}

}  // namespace metadatautil
