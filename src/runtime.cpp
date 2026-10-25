// SPDX-License-Identifier: GPL-2.0-or-later
#include "runtime.h"

#include <exiv2/exiv2.hpp>
#include <iostream>
#include <mutex>

namespace ddbexif {

namespace {

// The XMP toolkit calls this to serialize the namespaces it registers while decoding
void lockUnlock(void* data, bool lock) {
    auto* mutex = static_cast<std::mutex*>(data);
    if (lock)
        mutex->lock();
    else
        mutex->unlock();
}

std::mutex xmpMutex;

}  // namespace

bool initializeExiv2() {
    // Exiv2 reports warnings (a damaged tag, an unknown maker note) on stderr by default. Nothing
    // reads them, and stdout, which carries the responses, is not theirs: keep the log off.
    Exiv2::LogMsg::setLevel(Exiv2::LogMsg::mute);

    if (!Exiv2::XmpParser::initialize(lockUnlock, &xmpMutex)) {
        std::cerr << "ddb-exif: cannot initialize the XMP toolkit\n";
        return false;
    }

    try {
        // Thermal cameras write FLIR properties under this namespace; without it their keys
        // would not resolve
        Exiv2::XmpProperties::registerNs("http://ns.flir.com/xmp/1.0/", "FLIR");
    } catch (const Exiv2::Error& e) {
        std::cerr << "ddb-exif: cannot register the FLIR XMP namespace: " << e.what() << '\n';
        return false;
    }
    return true;
}

void shutdownExiv2() {
    Exiv2::XmpParser::terminate();
}

}  // namespace ddbexif
