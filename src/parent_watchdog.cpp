// SPDX-License-Identifier: GPL-2.0-or-later
#include "parent_watchdog.h"

#ifndef _WIN32

#include <unistd.h>

#include <chrono>
#include <thread>

namespace ddbexif {

void startParentWatchdog() {
    const pid_t parent = getppid();
    std::thread([parent] {
        for (;;) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            if (getppid() != parent)
                _exit(1);
        }
    }).detach();
}

}  // namespace ddbexif

#else

namespace ddbexif {

void startParentWatchdog() {}

}  // namespace ddbexif

#endif
