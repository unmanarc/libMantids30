#include "streamablejson.h"
#include <limits>
#include <iostream>

using namespace Mantids30::Memory::Streams;
using namespace Mantids30;

StreamableJSON::StreamableJSON()
{
    // No max size (original...)
    m_maxSize = std::numeric_limits<uint64_t>::max();
    setFormatted(false);
}

bool StreamableJSON::streamTo(std::shared_ptr<Memory::Streams::StreamableObject> out, Memory::Streams::StreamableObject::Status &wrStatUpd)
{
    Memory::Streams::StreamableObject::Status cur;
    if (!m_formatted)
        m_strValue = Mantids30::Helpers::jsonToString(m_root);
    else
        m_strValue = m_root.toStyledString();
    return (cur = out->writeFullStream(m_strValue.c_str(), m_strValue.size(), wrStatUpd)).succeed;
}

Memory::Streams::StreamableObject::Status StreamableJSON::write(const void *buf, const size_t &count, Memory::Streams::StreamableObject::Status &wrStatUpd)
{
    // TODO: how to report that the max size has been exceeded.
    Memory::Streams::StreamableObject::Status cur;

    // ...
    if ( m_strValue.size()+count > m_maxSize ) 
    {
        cur.bytesWritten = m_maxSize-m_strValue.size();
    }
    else
    {
        cur.bytesWritten = count;
    }

    if (cur.bytesWritten)
        m_strValue += std::string((static_cast<const char *>(buf)),cur.bytesWritten); // Copy...
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
    m_root = x;
    m_strValue.clear();
}

std::string StreamableJSON::getString()
{
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    return Json::writeString(builder, m_root);
}

json *StreamableJSON::processValue()
{
    Mantids30::Helpers::JSONReader2 reader;
    bool parsingSuccessful = reader.parse( m_strValue, m_root );
    if ( !parsingSuccessful )
        return nullptr;
    return &m_root;
}

json *StreamableJSON::getValue()
{
    return &m_root;
}

StreamableJSON &StreamableJSON::operator=(const Json::Value &value) {
    setValue(value);
    return *this;
}


void StreamableJSON::setValue(const json &value)
{
    m_root=value;
}

void StreamableJSON::setMaxSize(const uint64_t &value)
{
    m_maxSize = value;
}

bool StreamableJSON::getFormatted() const
{
    return m_formatted;
}

void StreamableJSON::setFormatted(bool value)
{
    m_formatted = value;
}
