#include "a_uint32.h"
#include <mutex>

using namespace Mantids30::Memory::Abstract;

UINT32::UINT32()
{
    setVarType(Type::UINT32);
}

UINT32::UINT32(const uint32_t &value)
{
    setVarType(Type::UINT32);
    this->m_value = value;
}

uint32_t UINT32::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    return m_value;
}

bool UINT32::setValue(const uint32_t &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string UINT32::toString()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    return std::to_string(m_value);
}

bool UINT32::fromString(const std::string &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    if (value.empty())
    {
        this->m_value = 0;
        return false;
    }

    char *end = nullptr;
    unsigned long result = strtoul(value.c_str(), &end, 10);

    if (end == value.c_str() || *end != '\0' || result > UINT32_MAX)
    {
        this->m_value = 0;
        return false;
    }

    this->m_value = static_cast<uint32_t>(result);
    return true;
}

std::shared_ptr<Var> UINT32::protectedCopy()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::shared_ptr<UINT32> var = std::make_shared<UINT32>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}

Json::Value UINT32::toJSON()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    if (isNull())
    {
        return Json::nullValue;
    }

    return m_value;
}

bool UINT32::fromJSON(const Json::Value &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_value = Helpers::JSON::ASUINT_D(value, 0);
    return true;
}
