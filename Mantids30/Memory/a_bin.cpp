#include "a_bin.h"
#include <string.h>
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

BINARY::BINARY()
{
    setVarType(TYPE_BIN);
}


BINARY::sBinContainer *BINARY::getValue()
{
    m_value.mutex.lock();
    return &m_value;
}

bool BINARY::setValue(sBinContainer *value)
{
    Threads::Sync::Lock_RW lock(this->m_value.mutex);
    this->m_value.ptr = new char[value->dataSize];
    if (!this->m_value.ptr) return false;
    this->m_value.dataSize = value->dataSize;
    memcpy(this->m_value.ptr, value->ptr, value->dataSize);
    return true;
}

std::string BINARY::toString()
{
    Threads::Sync::Lock_RD lock(this->m_value.mutex);
    std::string x( ((char *)m_value.ptr), m_value.dataSize);
    return x;
}

bool BINARY::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(this->m_value.mutex);
    this->m_value.ptr = new char[value.size()+1];
    if (!this->m_value.ptr) return false;
    this->m_value.ptr[value.size()] = 0;
    memcpy(this->m_value.ptr,value.c_str(),value.size());
    return true;
}

std::shared_ptr<Var> BINARY::protectedCopy()
{
    Threads::Sync::Lock_RD lock(this->m_value.mutex);

    auto var = std::make_shared<BINARY>();
    if (!var->setValue(&(this->m_value)))
    {
        return nullptr;
    }
    return var;
}
