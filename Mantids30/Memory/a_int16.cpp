#include "a_int16.h"
#include <mutex>
#include <cstdlib>

using namespace Mantids30::Memory::Abstract;

INT16::INT16()
{
    setVarType(Type::INT16);
}

INT16::INT16(const int16_t &value)
{
    this->m_value = value;

    setVarType(Type::INT16);
}

int16_t INT16::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    return m_value;
}

bool INT16::setValue(const int16_t &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string INT16::toString()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return std::to_string(m_value);
}

bool INT16::fromString(const std::string &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    if (value.empty())
    {
        this->m_value = 0;
        return false;
    }

    char *end = nullptr;
    long result = strtol(value.c_str(), &end, 10);

    if (end == value.c_str() || *end != '\0' || result > INT16_MAX || result < INT16_MIN)
    {
        this->m_value = 0;
        return false;
    }

    this->m_value = static_cast<int16_t>(result);
    return true;
}

std::shared_ptr<Var> INT16::protectedCopy()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::shared_ptr<INT16> var = std::make_shared<INT16>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}

Json::Value INT16::toJSON()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    if (isNull())
    {
        return Json::nullValue;
    }

    return m_value;
}

bool INT16::fromJSON(const Json::Value &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_value = Helpers::JSON::ASINT_D(value, 0);
    return true;
}
