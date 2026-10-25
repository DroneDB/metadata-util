// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

namespace metadatautil {

// Prepares Exiv2 for serving: keeps its log off stdout, initializes the XMP toolkit and registers
// the XMP namespaces DroneDB reads. Call it once, before the first request, and not for
// --version. Returns false (after telling stderr why) when that is not possible.
bool initializeExiv2();

// Releases what initializeExiv2 set up
void shutdownExiv2();

}  // namespace metadatautil
