#include "a_int8.h"
#include "Mantids30/Helpers/json.h"
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

INT8::INT8()
{
    setVarType(TYPE_INT8);

}

INT8::INT8(const int8_t &value)
{
    this->m_value = value;

    setVarType(TYPE_INT64);
}

int8_t INT8::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);
    return m_value;
}

bool INT8::setValue(const int8_t &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    this->m_value = value;
    return true;
}

std::string INT8::toString()
{
    Threads::Sync::Lock_RD lock(m_mutex);
    return std::to_string(m_value);
}

bool INT8::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    if (value.empty())
    {
        this->m_value = 0;
        return true;
    }
    this->m_value = static_cast<int8_t>(strtol(value.c_str(), nullptr, 10));
    if (value != "0" && this->m_value == 0)
        return false;

    return true;
}
std::shared_ptr<Var> INT8::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    auto var = std::make_shared<INT8>();
    if (var)
        *var = this->m_value;
    return var;
}

json INT8::toJSON()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    if (getIsNull())
        return Json::nullValue;

    return m_value;
}

bool INT8::fromJSON(const json &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    m_value = JSON_ASINT_D(value, 0);
    return true;
}
