// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <iosfwd>

#include "service.h"

namespace metadatautil {

// Reads request lines from `in` until end of input and writes one response line per request to
// `out`, in order, flushing after each. Returns 0 at end of input, 1 when `out` fails (the
// reader is gone).
int serve(std::istream& in, std::ostream& out, const MetadataService& service);

}  // namespace metadatautil
