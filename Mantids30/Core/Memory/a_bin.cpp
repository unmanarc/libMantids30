#include "a_bin.h"
#include <Mantids30/Core/Helpers/encoders.h>
#include <cstring>
#include <mutex>
#include <shared_mutex>

using namespace Mantids30::Core::Memory::Abstract;

BINARY::BINARY()
{
    setVarType(Type::BIN);
}

BINARY::ByteArray *BINARY::getValue()
{
    m_value.mutex.lock();
    return &m_value;
}

bool BINARY::setValue(ByteArray *value)
{
    std::unique_lock<std::shared_mutex> lock(this->m_value.mutex);
    // Copy before releasing the previous buffer so replacement also supports aliased input.
    char *replacement = new char[value->dataSize + 1];
    if (value->dataSize)
    {
        memcpy(replacement, value->ptr, value->dataSize);
    }
    replacement[value->dataSize] = '\0';
    delete[] m_value.ptr;
    m_value.ptr = replacement;
    m_value.dataSize = value->dataSize;
    return true;
}

std::string BINARY::toString()
{
    std::shared_lock<std::shared_mutex> lock(this->m_value.mutex);
    // An empty binary value may have no allocated buffer.
    if (m_value.dataSize == 0)
    {
        return "";
    }
    std::string x((m_value.ptr), m_value.dataSize);
    return x;
}

bool BINARY::fromString(std::string_view value)
{
    std::unique_lock<std::shared_mutex> lock(this->m_value.mutex);

    char *ptr = new (std::nothrow) char[value.size() + 1];
    if (!ptr)
    {
        return false;
    }

    if (!value.empty())
    {
        memcpy(ptr, value.data(), value.size());
    }
    ptr[value.size()] = '\0';

    delete[] this->m_value.ptr;
    this->m_value.ptr = ptr;
    // Keep the binary length independent of embedded null bytes.
    this->m_value.dataSize = value.size();

    return true;
}

boost::json::value BINARY::toJSON()
{
    std::shared_lock<std::shared_mutex> lock(this->m_value.mutex);

    if (isNull())
    {
        return boost::json::value{};
    }

    return boost::json::value(Mantids30::Core::Helpers::Encoders::encodeToBase64(reinterpret_cast<const unsigned char *>(m_value.ptr), m_value.dataSize).c_str());
}

bool BINARY::fromJSON(const boost::json::value &value)
{
    // fromString acquires the value mutex after decoding; do not lock it twice.
    return fromString(Mantids30::Core::Helpers::Encoders::decodeFromBase64( Mantids30::DataFormat::JSON::ASSTRING_D(value, "") ));
}

std::shared_ptr<Var> BINARY::protectedCopy()
{
    std::shared_lock<std::shared_mutex> lock(this->m_value.mutex);

    std::shared_ptr<BINARY> var = std::make_shared<BINARY>();
    if (!var->setValue(&(this->m_value)))
    {
        return nullptr;
    }
    return var;
}
