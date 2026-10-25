// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <string>
#include <vector>

namespace metadatautil {

// The program: `args` are the command line arguments after the program name, as UTF-8 (bytes on
// POSIX). Returns the exit status.
//
//   --version         one line with the versions, no setup
//   --serve           answers the requests of PROTOCOL.md on stdin/stdout
//   dump <file>...    prints the read response of each file, one line each
int runCli(const std::vector<std::string>& args);

}  // namespace metadatautil
