# ddb-exif protocol

**Protocol version 1.0**

SPDX-License-Identifier: CC0-1.0

This document is released under CC0 1.0 Universal (public domain dedication,
<https://creativecommons.org/publicdomain/zero/1.0/>). Anyone may implement either side of the
protocol, under any licence, without asking.

The protocol is the whole interface between `ddb-exif` and the programs that use it. It carries
plain data only: file paths, tag keys, Exiv2 type names, tag values and status. It carries no
structures, pointers, handles, file descriptors, shared memory names, numeric Exiv2 enumerations,
offsets or byte orders, and no request depends on an earlier one.

## 1. Transport

`ddb-exif --serve` reads requests from standard input and writes responses to standard output.

* A request or a response is one JSON object (RFC 8259), encoded as UTF-8, on one line that ends
  with a line feed (`\n`). A carriage return before the line feed is allowed in a request and
  ignored.
* The server answers every request line with exactly one response line, in the order of the
  requests, and flushes it. It handles one request at a time. A client may send several requests
  before it reads, as long as it keeps reading.
* Nothing but response lines is written to standard output. Diagnostics go to standard error,
  which clients should read and discard or log.
* The server exits with status 0 within one second after standard input reaches end of file. It
  also exits when the process that started it goes away (on POSIX systems it notices that its
  parent process changed; Windows hosts are expected to start it in a job object that is killed
  with the host).
* A request line longer than 16 MiB (16 777 216 bytes) is discarded up to its line feed and
  answered with `error: "request"`.

## 2. Command line

| Command | Behaviour |
| --- | --- |
| `ddb-exif --version` | Prints one line, `ddb-exif <version> (Exiv2 <exiv2 version>) protocol <major>.<minor>`, and exits 0. `<version>` is the version of the DroneDB build that contains the program (`unknown` for a standalone build). Nothing else is initialised. |
| `ddb-exif --serve` | Serves the requests of this document. File paths are never taken from the command line in this mode. |
| `ddb-exif dump <file>...` | Prints the `read` response of each file, one line per file, in argument order, then exits 0 if all of them were read and 1 if any failed. The `id` of each response is the 0-based position of the file among the arguments. A file that fails still prints an `ok: false` line. On Windows the arguments are read as UTF-16 and converted to UTF-8. |

The version line is matched by the regular expression
`protocol ([0-9]+)\.([0-9]+)$`. Clients should match it that way and ignore the rest of the line.

## 3. Requests

Every request is an object with an integer `id` (a signed 64-bit integer) and an `op`. Members a
request does not define are ignored, so that later minor versions can add optional members.

### 3.1 `read`

```json
{"id":7,"op":"read","path":"/data/flight/DJI_0042.JPG"}
```

| Member | Type | Meaning |
| --- | --- | --- |
| `path` | string | The file to read. See section 7 for `path64`. |

### 3.2 `write`

```json
{"id":8,"op":"write","path":"/data/out.tif","set":[{"key":"Exif.GPSInfo.GPSLatitude","value":"45/1 27/1 3036/100"},{"key":"Exif.GPSInfo.GPSLatitudeRef","value":"N"}]}
```

| Member | Type | Meaning |
| --- | --- | --- |
| `path` | string | The file to change (section 7 for `path64`). |
| `set` | array | Objects `{"key": <Exif key>, "value": <string>}`, applied in list order. `value` may be replaced by `value64` (section 7). A later entry for the same key overrides an earlier one. |

The server opens the file, reads its metadata, assigns every `set` entry the way Exiv2's
`exifData[key] = value` does (the string is parsed according to the key's default type), and writes
the file once. Only Exif keys can be set. If anything fails the request fails as a whole; Exiv2
writes through a temporary file where the format allows it, but a failed request does not
guarantee that the file is unchanged.

A request that does not match these shapes (a missing or non-integer `id`, an unknown `op`, a
missing path, a malformed `set`) is answered with `error: "request"`.

## 4. Responses

Every response has the `id` of its request (`null` when the request line could not be parsed far
enough to find one) and `ok`.

### 4.1 `read` success

```json
{"id":7,"ok":true,"width":5472,"height":3648,"exif":[ <tag>, ... ],"xmp":[ <tag>, ... ]}
```

| Member | Type | Meaning |
| --- | --- | --- |
| `width`, `height` | integer | `pixelWidth()` and `pixelHeight()` of the Exiv2 image after reading its metadata. 0 where the format does not tell. |
| `exif` | array | Every Exif datum, in the order Exiv2 iterates them. |
| `xmp` | array | Every XMP datum, in the order Exiv2 iterates them. |

Where a key appears more than once, the order is what lets a client pick the first.

### 4.2 `write` success

```json
{"id":8,"ok":true,"tags":[{"k":"Exif.GPSInfo.GPSLatitude","t":"Rational","n":3,"s":"45/1 27/1 3036/100"}]}
```

`tags` holds one object for each distinct key that was set, in the order of its first appearance in
`set`, as the file now holds it: `k`, `t`, `n` and `s` of section 5, without `v`.

### 4.3 Failure

```json
{"id":9,"ok":false,"error":"exiv2","message":"This does not look like a JPEG image"}
```

| `error` | Meaning |
| --- | --- |
| `request` | The request line is not a valid request (section 3). Not related to any file. |
| `open` | The file could not be opened or read: it does not exist, it is a directory, access is denied, or (for a write) the file could not be written. |
| `exiv2` | The file was opened but Exiv2 cannot read or write its content (not an image, damaged metadata, an invalid key or value in `set`), or something else went wrong while serving the request. |

`message` is a human-readable diagnostic of at most 2000 bytes. It is not part of the contract and
is always valid UTF-8 (ill-formed bytes are replaced by U+FFFD). A server never stops because of a
failed request; it answers and reads the next line.

## 5. Tag object

```json
{"k":"Exif.GPSInfo.GPSLatitude","t":"Rational","n":3,"s":"46/1 4/1 5129/1000","v":[[46,46.0,[46,1],true],[4,4.0,[4,1],true],[5,5.129,[5129,1000],true]]}
```

| Member | Type | Meaning |
| --- | --- | --- |
| `k` | string | `key()`, for example `Exif.GPSInfo.GPSLatitude` or `Xmp.drone-dji.RelativeAltitude`. `k64` when not valid UTF-8 (section 7). |
| `t` | string | `typeName()`, for example `Rational`, `Ascii`, `XmpText`. |
| `n` | integer | `count()`. |
| `s` | string | `toString()`. `s64` when not valid UTF-8 (section 7). Absent when `trunc` is true. |
| `v` | array | The component values of section 6. Absent when `trunc` is true, and in the tags of a `write` response. |
| `trunc` | boolean | Present, and true, only when `toString()` is longer than 65 536 bytes. Then `s`, `s64` and `v` are absent. |

## 6. Component values

`v` holds Exiv2's own conversion results for component `i`, for `i` from 0 to
`max(min(n, 64), 3) - 1`. There are at least three entries, even for a tag with fewer
components, because readers of coordinates (degrees, minutes, seconds) take the first three
components without checking `n`. There are never more than 64.

Each entry is `null` or an array of four members:

```
[ toInt64(i), toFloat(i), [ numerator, denominator ], ok ]
```

* `toInt64(i)` is a JSON integer.
* `toFloat(i)` is a JSON number holding the single-precision result as the shortest text that
  reads back as the same float after a client parsed it as a double and converted it to float
  (`5.0`, `0.1`, `-0.0`). A value that is not finite is the JSON string `"NaN"`, `"Infinity"` or
  `"-Infinity"`, since JSON has no such numbers.
* `[numerator, denominator]` is `toRational(i)`, two 32-bit signed integers.
* `ok` is true when none of the three conversions reported a failure through Exiv2's `ok()`
  (so, for an XMP text that is not a number, false).

The entry is `null` where Exiv2 throws `std::out_of_range` for component `i`. A client that
mirrors Exiv2 reports the same failure for `toInt64(i)`, `toFloat(i)` and `toRational(i)` there,
and for any `i` beyond the end of `v`. Which indices exist depends on the kind of value Exiv2
holds for the tag:

| Value family (type names) | Component `i` |
| --- | --- |
| Numeric (`Short`, `Long`, `Rational`, `SRational`, `SByte`, `SShort`, `SLong`, `Float`, `Double`, `Ifd`, `LongLong`, `SLongLong`, `Ifd8`) | Exists for `i < n`, `null` beyond. |
| Byte and bytes (`Byte`, `Undefined`) | Exists for `i < n` (the byte value), `null` beyond. |
| Text (`Ascii`, `String`, `Comment`) | Exists for `i < n`, `null` beyond. The conversions return the code of the character as a signed char: the text `DJI` gives `[68,68.0,[68,1],true]` for `i` = 0. |
| XMP arrays (`XmpAlt`, `XmpBag`, `XmpSeq`) | Exists for `i < n` (the array element, parsed as text); `null` beyond. |
| `XmpText` | The index is ignored: every entry is the conversion of the whole text, and `n` is the length of that text. A text that is not a number gives `0` and `ok` false. |
| `LangAlt` | The index is ignored: every entry is `[0,0.0,[0,0],false]`. |
| `Date`, `Time` | The index is ignored. |

The families that ignore the index repeat the same entry up to `min(n, 64)` times; a client may
store the first entry once. The table describes Exiv2 0.28; the normative value is always the
one Exiv2 returns.

## 7. Text that is not UTF-8

Metadata strings and file names are bytes, not always text. JSON strings must be valid UTF-8, so
a string field that is not stays intact like this: the field `<name>` is replaced by `<name>64`,
holding the standard base64 (RFC 4648 section 4, with padding) of the raw bytes. This is lossless
and works in both directions:

| Message | Fields |
| --- | --- |
| Request | `path` or `path64`, and the `value` of a `set` entry or `value64`. Exactly one of the two forms. |
| Response | `k` or `k64`, and `s` or `s64`, of a tag. |

A receiver accepts both forms for every field that has them. A sender uses the plain form when the
bytes are valid UTF-8 and the base64 form otherwise. Type names (`t`), error codes and the `message`
of a failure are always plain.

## 8. Paths

A path is taken as bytes by the file system (UTF-8 on Windows, converted to UTF-16 by the program).
A relative path is resolved against the working directory of the server, never against anything in
the request. Clients should send absolute paths: the working directory of the server is whatever it
was started with, which a client does not necessarily control. Whatever the form of the path, the
server opens that file and nothing else: names Exiv2 would treat as a URL or as standard input are
only ever files. The server opens a file for `read` and `write` requests only, and changes a file
for `write` requests only.

On Windows a path with non-ASCII characters, judged on the absolute path that is opened (a relative
ASCII path under a non-ASCII working directory counts), is read into memory and opened from there,
since Exiv2 opens files with the ANSI code page; a `write` writes the result back to the same path.

## 9. Limits

| Item | Limit |
| --- | --- |
| Request line | 16 MiB |
| `toString()` of a tag | 65 536 bytes (then `trunc`) |
| Component entries `v` | at least 3, at most 64 per tag |
| `message` of a failure | 2000 bytes |

A client should cap the length of a response line, and treat a longer one, a line that is not
valid JSON, or a response with an `id` it did not ask for as a broken server.

## 10. Versioning

The protocol version is `<major>.<minor>`, reported by `--version` and at the head of this
document.

* A change that existing clients cannot read raises the **major** number. That includes removing
  or renaming a member, changing its type or meaning, and changing which requests are accepted.
* A change that existing clients can read raises the **minor** number: new optional request
  members, new response members, new commands. Clients ignore response members they do not know.
  A new value of `error` is not such a change, since a client has to treat each value in its own
  way.
* A client implementing version `C.m` accepts a server of version `S.n` only when `S` = `C` and
  `n` >= `m`. A client checks the version once, when it starts a server, with `--version`.

## 11. Example

```
{"id":7,"op":"read","path":"/data/flight/DJI_0042.JPG"}
{"id":7,"ok":true,"width":5472,"height":3648,"exif":[{"k":"Exif.Image.Make","t":"Ascii","n":4,"s":"DJI","v":[[68,68.0,[68,1],true],[74,74.0,[74,1],true],[73,73.0,[73,1],true],[0,0.0,[0,1],true]]}],"xmp":[]}
{"id":8,"op":"write","path":"/data/out.tif","set":[{"key":"Exif.GPSInfo.GPSLatitudeRef","value":"N"}]}
{"id":8,"ok":true,"tags":[{"k":"Exif.GPSInfo.GPSLatitudeRef","t":"Ascii","n":2,"s":"N"}]}
{"id":9,"op":"read","path":"/data/not-an-image.jpg"}
{"id":9,"ok":false,"error":"exiv2","message":"corrupted image metadata"}
not json
{"id":null,"ok":false,"error":"request","message":"not a valid JSON line: ..."}
```

(The first response is shortened to one tag.)
