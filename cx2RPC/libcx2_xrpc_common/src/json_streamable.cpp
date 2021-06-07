#include "json_streamable.h"
#include <limits>
#include <iostream>

using namespace CX2::Memory::Streams;
using namespace CX2;

JSON_Streamable::JSON_Streamable()
{
    // No max size (original...)
    maxSize = std::numeric_limits<uint64_t>::max();
    setFormatted(false);
}

std::string JSON_Streamable::jsonToString(const json &value)
{
    Json::FastWriter fastWriter;
    std::string xstrValue = fastWriter.write(value);
    if (!xstrValue.empty() && xstrValue[xstrValue.length()-1] == '\n')
    {
        xstrValue.erase(xstrValue.length()-1);
    }
    return xstrValue;
}


bool JSON_Streamable::streamTo(Memory::Streams::Streamable *out, Memory::Streams::Status &wrStatUpd)
{
    Memory::Streams::Status cur;
    if (!formatted)
        strValue = jsonToString(root);
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
    //root.clear();
    strValue.clear();
}

std::string JSON_Streamable::getString()
{
    Json::FastWriter fastWriter;
    return fastWriter.write(root);
}

json *JSON_Streamable::processValue()
{
    Json::Reader reader;
    bool parsingSuccessful = reader.parse( strValue, root );
    if ( !parsingSuccessful )
        return nullptr;
    return &root;
}

json *JSON_Streamable::getValue()
{
  //  std::cout << "getting value" << std::endl << std::flush;
    return &root;
}

void JSON_Streamable::setValue(const json &value)
{
//    std::cout << "setting value to " << value.toStyledString() << std::endl << std::flush;
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
