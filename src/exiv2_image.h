// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <exiv2/exiv2.hpp>
#include <stdexcept>
#include <string>

// The only place that opens and saves image files through Exiv2.
namespace ddbexif {

// The file could not be opened or read, before Exiv2 looked at its content
class OpenError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Opens the image at `path` (bytes: UTF-8, or anything the file system takes on POSIX).
//
// A relative path is made absolute first: Exiv2 reads "-" as standard input and "http://..." as a
// download, and the file named by a request must be the only thing opened.
//
// On Windows a path with non-ASCII characters is read into memory and opened from there, because
// Exiv2 opens files with the ANSI code page. Throws OpenError, or Exiv2::Error when the content is
// not an image.
Exiv2::Image::UniquePtr openImage(const std::string& path);

// Writes the metadata of `image` (opened by openImage from `path`) to the file. On Windows, for an
// image held in memory, that writes the result back to `path`.
void saveImage(Exiv2::Image& image, const std::string& path);

// Whether `error` says that the file could not be opened, as opposed to being unreadable content
bool isOpenFailure(const Exiv2::Error& error);

}  // namespace ddbexif
