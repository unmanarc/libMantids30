#include "a_bin.h"
#include <string.h>
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Memory::Abstract;

BINARY::BINARY()
{
    setVarType(TYPE_BIN);
}

BINARY::~BINARY()
{

}

BINARY::sBinContainer *BINARY::getValue()
{
    value.mutex.lock();
    return &value;
}

bool BINARY::setValue(sBinContainer *value)
{
    Threads::Sync::Lock_RW lock(this->value.mutex);
    this->value.ptr = new char[value->dataSize];
    if (!this->value.ptr) return false;
    this->value.dataSize = value->dataSize;
    memcpy(this->value.ptr, value->ptr, value->dataSize);
    return true;
}

std::string BINARY::toString()
{
    Threads::Sync::Lock_RD lock(this->value.mutex);
    std::string x( ((char *)value.ptr), value.dataSize);
    return x;
}

bool BINARY::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(this->value.mutex);
    this->value.ptr = new char[value.size()+1];
    if (!this->value.ptr) return false;
    this->value.ptr[value.size()] = 0;
    memcpy(this->value.ptr,value.c_str(),value.size());
    return true;
}

std::shared_ptr<Var> BINARY::protectedCopy()
{
    Threads::Sync::Lock_RD lock(this->value.mutex);

    auto var = std::make_shared<BINARY>();
    if (!var->setValue(&(this->value)))
    {
        return nullptr;
    }
    return var;
}
