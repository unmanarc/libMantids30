#include "a_bool.h"
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

BOOL::BOOL()
{
    setVarType(TYPE_BOOL);

}

BOOL::BOOL(const bool &value)
{
    setVarType(TYPE_BOOL);
    this->m_value = value;

}

bool BOOL::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);
    return m_value;
}

bool BOOL::setValue(bool value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    this->m_value = value;
    return true;
}

std::string BOOL::toString()
{
    Threads::Sync::Lock_RD lock(m_mutex);
    return m_value ? "true" : "false";
}

bool BOOL::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    if (value == "true" || value == "TRUE" || value == "1" || value == "t" || value == "T")
        this->m_value = true;
    else
        this->m_value = false;
    return true;
}

json BOOL::toJSON()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    if (isNull())
        return Json::nullValue;

    return m_value;
}

bool BOOL::fromJSON(const json &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    m_value = JSON_ASBOOL_D(value, false);
    return true;
}

std::shared_ptr<Var> BOOL::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    auto var = std::make_shared<BOOL>();
    if (var)
        *var = this->m_value;
    return var;
}
