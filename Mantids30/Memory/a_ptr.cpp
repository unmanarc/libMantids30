#include "a_ptr.h"
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

PTR::PTR()
{
    setVarType(TYPE_PTR);
}

PTR::PTR(void *value)
{
    setVarType(TYPE_PTR);
    this->m_value = value;
}

void * PTR::getValue()
{
    Threads::Sync::Lock_RD lock(m_mutex);
    return m_value;
}

bool PTR::setValue(void * value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string PTR::toString()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    char ovalue[256];
    void * ptr = m_value;
    snprintf(ovalue,sizeof(ovalue),"%.8lX", (uintptr_t)ptr);
    return ovalue;
}

bool PTR::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    if (value.empty())
    {
        this->m_value = 0;
        return true;
    }
    this->m_value = (void *)(strtol( value.c_str(), nullptr, 16 ));
    return true;
}

std::shared_ptr<Var> PTR::protectedCopy()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    auto var = std::make_shared<PTR>();
    if (var) *var = this->m_value;
    return var;
}

