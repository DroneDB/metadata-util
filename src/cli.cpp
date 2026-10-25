// SPDX-License-Identifier: GPL-2.0-or-later
#include "cli.h"

#include <exiv2/exiv2.hpp>
#include <iostream>

#include "parent_watchdog.h"
#include "protocol_version.h"
#include "runtime.h"
#include "server.h"
#include "service.h"
#include "version_info.h"

namespace ddbexif {

namespace {

int printUsage(std::ostream& out, int status) {
    out << "Usage: ddb-exif --version\n"
           "       ddb-exif --serve\n"
           "       ddb-exif dump <file>...\n";
    return status;
}

int printVersion() {
    std::cout << "ddb-exif " << DDB_EXIF_VERSION_STRING << " (Exiv2 " << Exiv2::versionString() << ") protocol "
              << ProtocolMajor << '.' << ProtocolMinor << std::endl;
    return std::cout ? 0 : 1;
}

int runServe() {
    if (!initializeExiv2())
        return 2;
    startParentWatchdog();

    const int status = serve(std::cin, std::cout, MetadataService());
    shutdownExiv2();
    return status;
}

int runDump(const std::vector<std::string>& files) {
    if (!initializeExiv2())
        return 2;

    const MetadataService service;
    bool allRead = true;
    int64_t id = 0;
    for (const auto& file : files) {
        const Response response = service.read(id++, file);
        std::cout << response.line << '\n';
        allRead = allRead && response.ok;
    }
    std::cout << std::flush;
    shutdownExiv2();
    return allRead && std::cout ? 0 : 1;
}

}  // namespace

int runCli(const std::vector<std::string>& args) {
    // Lines are small and flushed by hand: no coupling with C stdio
    std::ios::sync_with_stdio(false);

    if (args.size() == 1 && args[0] == "--version")
        return printVersion();
    if (args.size() == 1 && args[0] == "--serve")
        return runServe();
    if (args.size() >= 2 && args[0] == "dump")
        return runDump(std::vector<std::string>(args.begin() + 1, args.end()));
    if (args.size() == 1 && (args[0] == "--help" || args[0] == "-h"))
        return printUsage(std::cout, 0);
    return printUsage(std::cerr, 2);
}

}  // namespace ddbexif
