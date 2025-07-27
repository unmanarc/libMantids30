#include "streamablestring.h"
#include <optional>
#include <string.h>

using namespace Mantids30::Memory::Streams;

bool StreamableString::streamTo(Memory::Streams::StreamableObject *out)
{
    return out->writeFullStreamWithEOF(m_value.c_str(), m_value.size());
}

std::optional<size_t> StreamableString::write(const void *buf, const size_t &count)
{
    try
    {
        std::string x((static_cast<const char *>(buf)), count);
        m_value += x;
        writeStatus += count;
        return count;
    }
    catch (const std::string &exception)
    {
        // Set error:
        writeStatus+=-1;
        return std::nullopt;
    }
}

StreamableString &StreamableString::operator=(
    const std::string &str)
{
    setValue(str);
    return *this;
}

const std::string &StreamableString::getValue() const
{
    return m_value;
}

void StreamableString::setValue(
    const std::string &newValue)
{
    m_value = newValue;
}
