#include "a_bin.h"
#include "Mantids30/Helpers/encoders.h"
#include "json/value.h"
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
    if (!this->m_value.ptr) 
        return false;
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
    if (!this->m_value.ptr)
        return false;
    this->m_value.ptr[value.size()] = 0;
    memcpy(this->m_value.ptr,value.c_str(),value.size());
    return true;
}

json BINARY::toJSON()
{
    Threads::Sync::Lock_RD lock(this->m_value.mutex);

    if (getIsNull())
        return Json::nullValue;

    return Helpers::Encoders::encodeToBase64(m_value.ptr,m_value.dataSize);
}

bool BINARY::fromJSON(const json &value)
{
    Threads::Sync::Lock_RW lock(this->m_value.mutex);
    return fromString(Helpers::Encoders::decodeFromBase64(JSON_ASSTRING_D(value,"")));
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
