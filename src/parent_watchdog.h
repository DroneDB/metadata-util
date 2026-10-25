// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

namespace ddbexif {

// Ends the process when the process that started it goes away, even in the middle of a request:
// a host that was killed cannot close our stdin in an orderly way. A background thread polls the
// parent process id twice a second; the id changes when the parent dies.
//
// Windows hosts put the helper in a job that is killed with the host, so there it does nothing.
void startParentWatchdog();

}  // namespace ddbexif
