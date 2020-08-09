#include "a_bin.h"
#include <string.h>
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Memory::Vars;

A_BIN::A_BIN()
{
    setVarType(ABSTRACT_BIN);
}

A_BIN::~A_BIN()
{

}

sBinContainer *A_BIN::getValue()
{
    value.mutex.lock();
    return &value;
}

bool A_BIN::setValue(sBinContainer *value)
{
    Threads::Sync::Lock_RW lock(this->value.mutex);
    this->value.ptr = new char[value->dataSize];
    if (!this->value.ptr) return false;
    this->value.dataSize = value->dataSize;
    memcpy(this->value.ptr, value->ptr, value->dataSize);
    return true;
}

std::string A_BIN::toString()
{
    Threads::Sync::Lock_RD lock(this->value.mutex);
    std::string x( ((char *)value.ptr), value.dataSize);
    return x;
}

bool A_BIN::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(this->value.mutex);
    this->value.ptr = new char[value.size()+1];
    if (!this->value.ptr) return false;
    this->value.ptr[value.size()] = 0;
    memcpy(this->value.ptr,value.c_str(),value.size());
    return true;
}

Abstract *A_BIN::protectedCopy()
{
    Threads::Sync::Lock_RD lock(this->value.mutex);
    A_BIN * var = new A_BIN;
    if (!var->setValue(&(this->value)))
    {
        delete var;
        return nullptr;
    }
    return var;
}
