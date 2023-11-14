#include "streamablejson.h"
#include <limits>
#include <iostream>

using namespace Mantids30::Memory::Streams;
using namespace Mantids30;

StreamableJSON::StreamableJSON()
{
    // No max size (original...)
    maxSize = std::numeric_limits<uint64_t>::max();
    setFormatted(false);
}

bool StreamableJSON::streamTo(Memory::Streams::StreamableObject *out, Memory::Streams::StreamableObject::Status &wrStatUpd)
{
    Memory::Streams::StreamableObject::Status cur;
    if (!formatted)
        strValue = Mantids30::Helpers::jsonToString(root);
    else
        strValue = root.toStyledString();
    return (cur = out->writeFullStream(strValue.c_str(), strValue.size(), wrStatUpd)).succeed;
}

Memory::Streams::StreamableObject::Status StreamableJSON::write(const void *buf, const size_t &count, Memory::Streams::StreamableObject::Status &wrStatUpd)
{
    // TODO: how to report that the max size has been exceeded.
    Memory::Streams::StreamableObject::Status cur;

    // ...
    if ( strValue.size()+count > maxSize ) cur.bytesWritten = maxSize-strValue.size();
    else                                   cur.bytesWritten = count;

    if (cur.bytesWritten)
        strValue += std::string(((const char *)buf),cur.bytesWritten); // Copy...
    else
        wrStatUpd.finish = cur.finish = true;

    // Append...
    wrStatUpd.bytesWritten+=cur.bytesWritten;

    return cur;
}

void StreamableJSON::writeEOF(bool)
{
    processValue();
}

void StreamableJSON::clear()
{
    json x;
    root = x;
    strValue.clear();
}

std::string StreamableJSON::getString()
{
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    return Json::writeString(builder, root);
}

json *StreamableJSON::processValue()
{
    Mantids30::Helpers::JSONReader2 reader;
    bool parsingSuccessful = reader.parse( strValue, root );
    if ( !parsingSuccessful )
        return nullptr;
    return &root;
}

json *StreamableJSON::getValue()
{
    return &root;
}

StreamableJSON &StreamableJSON::operator=(const Json::Value &value) {
    setValue(value);
    return *this;
}


void StreamableJSON::setValue(const json &value)
{
    root=value;
}

void StreamableJSON::setMaxSize(const uint64_t &value)
{
    maxSize = value;
}

bool StreamableJSON::getFormatted() const
{
    return formatted;
}

void StreamableJSON::setFormatted(bool value)
{
    formatted = value;
}
