// SPDX-License-Identifier: GPL-2.0-or-later
#include "exiv2_image.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

namespace fs = std::filesystem;

namespace ddbexif {

namespace {

fs::path toPath(const std::string& bytes) {
#ifdef _WIN32
    return fs::u8path(bytes);
#else
    return fs::path(bytes);
#endif
}

fs::path toAbsolute(const std::string& bytes) {
    try {
        const fs::path path = toPath(bytes);
        return path.is_absolute() ? path : fs::absolute(path);
    } catch (const fs::filesystem_error& e) {
        throw OpenError(e.what());
    }
}

#ifdef _WIN32
bool isAscii(const std::string& text) {
    return std::all_of(text.begin(), text.end(), [](unsigned char c) { return c < 0x80; });
}
#endif

}  // namespace

Exiv2::Image::UniquePtr openImage(const std::string& path) {
    const fs::path absolute = toAbsolute(path);
    // Exiv2 reads a directory as damaged content; say what is wrong instead
    std::error_code ec;
    if (fs::is_directory(absolute, ec))
        throw OpenError(path + " is a directory");
#ifdef _WIN32
    if (!isAscii(path)) {
        std::ifstream in(absolute, std::ios::binary);
        if (!in)
            throw OpenError("Cannot open " + path);
        const std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (in.bad())
            throw OpenError("Cannot read " + path);

        auto memory = std::make_unique<Exiv2::MemIo>();
        memory->write(reinterpret_cast<const Exiv2::byte*>(data.data()), data.size());
        // Like the path overload (the one used for an ASCII path and on every other platform):
        // data of no known image type is an Exiv2::Error, where the BasicIo overload returns nullptr
        auto image = Exiv2::ImageFactory::open(std::move(memory));
        if (!image)
            throw Exiv2::Error(Exiv2::ErrorCode::kerFileContainsUnknownImageType, path);
        return image;
    }
    return Exiv2::ImageFactory::open(absolute.string());
#else
    return Exiv2::ImageFactory::open(absolute.native());
#endif
}

void saveImage(Exiv2::Image& image, const std::string& path) {
    image.writeMetadata();
#ifdef _WIN32
    if (auto* memory = dynamic_cast<Exiv2::MemIo*>(&image.io())) {
        std::ofstream out(toAbsolute(path), std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(memory->mmap()), static_cast<std::streamsize>(memory->size()));
        if (!out)
            throw OpenError("Cannot write " + path);
    }
#else
    (void)path;
#endif
}

bool isOpenFailure(const Exiv2::Error& error) {
    return error.code() == Exiv2::ErrorCode::kerDataSourceOpenFailed ||
           error.code() == Exiv2::ErrorCode::kerFileOpenFailed;
}

}  // namespace ddbexif
