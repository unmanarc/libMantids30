#include "json_streamable.h"
#include <limits>
#include <iostream>

using namespace Mantids::Memory::Streams;
using namespace Mantids;

JSON_Streamable::JSON_Streamable()
{
    // No max size (original...)
    maxSize = std::numeric_limits<uint64_t>::max();
    setFormatted(false);
}



bool JSON_Streamable::streamTo(Memory::Streams::Streamable *out, Memory::Streams::Status &wrStatUpd)
{
    Memory::Streams::Status cur;
    if (!formatted)
        strValue = Mantids::Helpers::jsonToString(root);
    else
        strValue = root.toStyledString();
    return (cur = out->writeFullStream(strValue.c_str(), strValue.size(), wrStatUpd)).succeed;
}

Memory::Streams::Status JSON_Streamable::write(const void *buf, const size_t &count, Memory::Streams::Status &wrStatUpd)
{
    // TODO: how to report that the max size has been exceeded.
    Memory::Streams::Status cur;

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

void JSON_Streamable::writeEOF(bool)
{
    processValue();
}

void JSON_Streamable::clear()
{
    json x;
    root = x;
    strValue.clear();
}

std::string JSON_Streamable::getString()
{
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    return Json::writeString(builder, root);
}

json *JSON_Streamable::processValue()
{
    Mantids::Helpers::JSONReader2 reader;
    bool parsingSuccessful = reader.parse( strValue, root );
    if ( !parsingSuccessful )
        return nullptr;
    return &root;
}

json *JSON_Streamable::getValue()
{
    return &root;
}

void JSON_Streamable::setValue(const json &value)
{
    root=value;
}

void JSON_Streamable::setMaxSize(const uint64_t &value)
{
    maxSize = value;
}

bool JSON_Streamable::getFormatted() const
{
    return formatted;
}

void JSON_Streamable::setFormatted(bool value)
{
    formatted = value;
}
