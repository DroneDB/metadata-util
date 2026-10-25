# metadata-util

`metadata-util` reads and writes image metadata (Exif and XMP) with [Exiv2](https://exiv2.org) for
[DroneDB](https://github.com/DroneDB/DroneDB). DroneDB runs it as a separate process and talks to
it over a line-based JSON protocol, so the Exiv2 code (GPL-2.0-or-later) stays in this program and
does not become part of DroneDB's libraries.

It also works on its own, like `exiv2 -pa`:

```
$ metadata-util dump DJI_0042.JPG
{"id":0,"ok":true,"width":5472,"height":3648,"exif":[{"k":"Exif.Image.Make", ...
```

## Usage

```
metadata-util --version          # metadata-util <version> (Exiv2 <version>) protocol <major>.<minor>
metadata-util --serve            # answer the requests of PROTOCOL.md on stdin/stdout
metadata-util dump <file>...     # one read response line per file; exit 1 if any file failed
```

The protocol is described in [PROTOCOL.md](PROTOCOL.md).

## Build

You need a C++17 compiler, CMake 3.16 or later, Exiv2 0.28 with the `bmff`, `png` and `xmp`
features (vcpkg port `exiv2`, same as DroneDB) and nlohmann_json.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

`-DMETADATA_UTIL_VERSION=<version>` sets the version `--version` prints (`unknown` by default). When the
directory is added with `add_subdirectory` from the DroneDB build, DroneDB's `APP_VERSION` is used
and the executable goes where the parent's `CMAKE_RUNTIME_OUTPUT_DIRECTORY` says.

The program does not link DroneDB or any code of it.

## Licence

* The source code is licensed under the GNU General Public License, version 2 or (at your option)
  any later version (GPL-2.0-or-later), because it links Exiv2. See [LICENSE](LICENSE).
* [PROTOCOL.md](PROTOCOL.md) is released under CC0-1.0 (public domain): anyone can implement a
  client or a server of the protocol under any licence.
