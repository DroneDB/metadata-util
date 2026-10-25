// SPDX-License-Identifier: GPL-2.0-or-later
#include "tag_serializer.h"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>

#include "protocol.h"

namespace ddbexif {

namespace {

struct Component {
    int64_t integer = 0;
    float real = 0.0F;
    Exiv2::Rational rational;
    bool ok = false;
};

// What Exiv2 returns for component `index`, with the three conversions the client mirrors.
// Empty where Exiv2 says the component does not exist (std::out_of_range): the same call on the
// client then fails the same way.
template <typename Datum>
std::optional<Component> convert(const Datum& datum, size_t index) {
    Component c;
    try {
        c.integer = datum.toInt64(index);
        bool ok = datum.value().ok();
        c.real = datum.toFloat(index);
        ok = datum.value().ok() && ok;
        c.rational = datum.toRational(index);
        c.ok = datum.value().ok() && ok;
    } catch (const std::out_of_range&) {
        return std::nullopt;
    }
    return c;
}

// How a value family treats the component index of toInt64(i), toFloat(i) and toRational(i),
// from Exiv2's value.cpp / value.hpp. Asking Exiv2 for the components it has not got costs a
// thrown exception each (about 1 us, most of the time of a read), and asking a family that
// ignores the index repeats the same text parse for every component. Knowing the rule saves that
// without changing a result; components of a family not listed are always asked for.
enum class IndexRule {
    // The component exists exactly when i < count(); beyond that std::out_of_range
    Bounded,
    // Every index gives the result of index 0
    Ignored,
    // Ask Exiv2
    Unknown,
};

IndexRule indexRule(Exiv2::TypeId type) {
    switch (type) {
        case Exiv2::unsignedByte:
        case Exiv2::asciiString:
        case Exiv2::unsignedShort:
        case Exiv2::unsignedLong:
        case Exiv2::unsignedRational:
        case Exiv2::signedByte:
        case Exiv2::undefined:
        case Exiv2::signedShort:
        case Exiv2::signedLong:
        case Exiv2::signedRational:
        case Exiv2::tiffFloat:
        case Exiv2::tiffDouble:
        case Exiv2::tiffIfd:
        case Exiv2::unsignedLongLong:
        case Exiv2::signedLongLong:
        case Exiv2::tiffIfd8:
        case Exiv2::string:
        case Exiv2::xmpAlt:
        case Exiv2::xmpBag:
        case Exiv2::xmpSeq:
            return IndexRule::Bounded;
        case Exiv2::date:
        case Exiv2::time:
        case Exiv2::xmpText:
        case Exiv2::langAlt:
            return IndexRule::Ignored;
        default:
            return IndexRule::Unknown;
    }
}

template <typename Datum>
void writeComponents(JsonWriter& writer, const Datum& datum, size_t count) {
    const IndexRule rule = indexRule(datum.typeId());
    const size_t entries = std::max(std::min(count, limits::MaxComponentValues), limits::MinComponentValues);

    std::optional<Component> first;
    writer.key("v").beginArray();
    for (size_t i = 0; i < entries; ++i) {
        std::optional<Component> c;
        if (rule == IndexRule::Bounded && i >= count)
            c = std::nullopt;
        else if (rule == IndexRule::Ignored && i > 0)
            c = first;
        else
            c = convert(datum, i);
        if (i == 0)
            first = c;

        if (!c) {
            writer.null();
            continue;
        }
        writer.beginArray();
        writer.integer(c->integer);
        writer.real(c->real);
        writer.beginArray().integer(c->rational.first).integer(c->rational.second).endArray();
        writer.boolean(c->ok);
        writer.endArray();
    }
    writer.endArray();
}

template <typename Datum>
void writeTagImpl(JsonWriter& writer, const Datum& datum, TagDetail detail) {
    const size_t count = datum.count();
    const std::string text = datum.toString();
    const char* typeName = datum.typeName();

    writer.beginObject();
    writeTextField(writer, "k", datum.key());
    writer.key("t").string(typeName ? typeName : "");
    writer.key("n").unsignedInteger(count);
    if (text.size() > limits::MaxTagStringBytes) {
        writer.key("trunc").boolean(true);
    } else {
        writeTextField(writer, "s", text);
        if (detail == TagDetail::Full)
            writeComponents(writer, datum, count);
    }
    writer.endObject();
}

}  // namespace

void writeTag(JsonWriter& writer, const Exiv2::Exifdatum& datum, TagDetail detail) {
    writeTagImpl(writer, datum, detail);
}

void writeTag(JsonWriter& writer, const Exiv2::Xmpdatum& datum, TagDetail detail) {
    writeTagImpl(writer, datum, detail);
}

}  // namespace ddbexif
