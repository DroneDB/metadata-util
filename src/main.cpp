// SPDX-License-Identifier: GPL-2.0-or-later
#include <string>
#include <vector>

#include "cli.h"

#ifdef _WIN32

#include <fcntl.h>
#include <io.h>

#include <cstdio>

#include "utf8.h"

// Arguments arrive as UTF-16, and paths are the point of the program
int wmain(int argc, wchar_t** argv) {
    // The protocol is lines ending in a bare line feed
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.push_back(ddbexif::utf8::fromWide(argv[i]));
    return ddbexif::runCli(args);
}

#else

int main(int argc, char** argv) {
    return ddbexif::runCli(std::vector<std::string>(argv + 1, argv + argc));
}

#endif
