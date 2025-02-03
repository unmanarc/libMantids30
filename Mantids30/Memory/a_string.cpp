#include "a_string.h"
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

STRING::STRING()
{
    setVarType(TYPE_STRING);
}

STRING::STRING(const std::string &value)
{
    setVarType(TYPE_STRING);
    setValue(value);
}



std::string STRING::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);
    return m_value;
}

bool STRING::setValue(const std::string &value)
{
    return fromString(value);
}

bool STRING::setValue(const char *value)
{
    if (!value)
        return fromString("");

    return fromString(value);
}

std::string STRING::toString()
{
    return getValue();
}

bool STRING::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);
    this->m_value = value;
    return true;
}

std::shared_ptr<Var> STRING::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    auto var = std::make_shared<STRING>();
    if (var) *var = this->m_value;
    return var;
}
