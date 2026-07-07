#include "a_bin.h"
#include <Mantids30/Helpers/encoders.h>
#include <shared_mutex>
#include <mutex>
#include <cstring>
#include <json/value.h>

using namespace Mantids30::Memory::Abstract;

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
    this->m_value.ptr = new char[value->dataSize];
    if (!this->m_value.ptr)
    {
        return false;
    }
    this->m_value.dataSize = value->dataSize;
    memcpy(this->m_value.ptr, value->ptr, value->dataSize);
    return true;
}

std::string BINARY::toString()
{
    std::shared_lock<std::shared_mutex> lock(this->m_value.mutex);
    std::string x((m_value.ptr), m_value.dataSize);
    return x;
}

bool BINARY::fromString(const std::string &value)
{
    std::unique_lock<std::shared_mutex> lock(this->m_value.mutex);

    this->m_value.ptr = new char[value.size() + 1];
    if (!this->m_value.ptr)
    {
        return false;
    }

    this->m_value.ptr[value.size()] = 0;
    memcpy(this->m_value.ptr, value.c_str(), value.size());
    return true;
}

Json::Value BINARY::toJSON()
{
    std::shared_lock<std::shared_mutex> lock(this->m_value.mutex);

    if (isNull())
    {
        return Json::nullValue;
    }

    return Helpers::Encoders::encodeToBase64(m_value.ptr, m_value.dataSize);
}

bool BINARY::fromJSON(const Json::Value &value)
{
    std::unique_lock<std::shared_mutex> lock(this->m_value.mutex);
    return fromString(Helpers::Encoders::decodeFromBase64(Helpers::JSON::ASSTRING_D(value, "")));
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
