#include "streamable_json.h"
#include <Mantids30/Core/Memory/streamable_null.h>
#include <Mantids30/Core/Memory/streamable_object.h>
#include <optional>

using namespace Mantids30::Core::Memory::Streams;
using namespace Mantids30::DataFormats::JSON;
using namespace Mantids30;

bool StreamableJSON::streamTo(Mantids30::Core::Memory::Streams::StreamableObject *out)
{
    Mantids30::Core::Memory::Streams::WriteStatus cur;

    if (!m_isFormatted)
    {
        m_strValue = Mantids30::DataFormats::JSON::toString(m_root);
    }
    else
    {
        // boost::json::serialize produces compact JSON (no trailing newline).
        m_strValue = boost::json::serialize(m_root);
    }

    return out->writeFullStream(m_strValue.c_str(), m_strValue.size());
}

std::optional<size_t> StreamableJSON::write(const void *buf, const size_t &count)
{
    ssize_t writtenBytes;

    if (count == 0)
    {
        // EOF:
        writeStatus += 0;
        if (!processValue())
        {
            writeStatus += -1;
            return std::nullopt;
        }
        return 0;
    }

    // ...
    if (m_strValue.size() + count > m_maxSize)
    {
        // Container Full! Can't process this information.
        // There is no sense to process an incomplete JSON.
        m_isFull = true;
        writeStatus += -1;
        return std::nullopt;
    }
    else
    {
        writtenBytes = count;
    }

    m_strValue += std::string((static_cast<const char *>(buf)), writtenBytes); // Copy...

    // Append...
    writeStatus += writtenBytes;

    return writtenBytes;
}

size_t StreamableJSON::size()
{
    StreamableNull sum;
    if (streamTo(&sum))
    {
        return sum.size();
    }
    return 0;
}

void StreamableJSON::clear()
{
    boost::json::value x;
    m_root = x;
    m_strValue.clear();
}

boost::json::value *StreamableJSON::processValue()
{
    if (m_isFull)
    {
        return nullptr;
    }

    Mantids30::DataFormats::JSON::Reader reader;
    bool parsingSuccessful = reader.parse(m_strValue, m_root);
    if (!parsingSuccessful)
    {
        return nullptr;
    }
    return &m_root;
}

boost::json::value *StreamableJSON::getValue()
{
    return &m_root;
}

bool StreamableJSON::isEmpty()
{
    return m_root.is_null();
}

StreamableJSON &StreamableJSON::operator=(const boost::json::value &value)
{
    setValue(value);
    return *this;
}

void StreamableJSON::setValue(const boost::json::value &value)
{
    m_root = value;
}

bool StreamableJSON::setValue(const std::string &value)
{
    Mantids30::DataFormats::JSON::Reader reader;
    m_strValue = value;
    bool parsingSuccessful = reader.parse(m_strValue, m_root);
    return parsingSuccessful;
}

void StreamableJSON::setMaxSize(const size_t &value)
{
    m_maxSize = value;
}

bool StreamableJSON::getIsFormatted() const
{
    return m_isFormatted;
}

void StreamableJSON::setIsFormatted(bool value)
{
    m_isFormatted = value;
}
