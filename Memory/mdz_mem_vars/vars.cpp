#include "vars.h"

using namespace Mantids::Memory::Abstract;

Vars::Vars()
{
    maxVarNameSize = 256; // 256 bytes
    maxVarContentSize = 128*1024; // 128Kb.
}

Vars::~Vars()
{
}

std::string Vars::getStringValue(const std::string &varName)
{
    auto * value = getValue(varName);
    return !value?"":value->toString();
}

std::list<std::string> Vars::getStringValues(const std::string &varName)
{
    std::list<std::string> r;
    auto contList = getValues(varName);
    for (auto * b : contList)
        r.push_back(b->toString());
    return r;
}

bool Vars::exist(const std::string &varName)
{
    return getValue(varName)!=nullptr?true:false;
}

uint32_t Vars::getMaxVarNameSize() const
{
    return maxVarNameSize;
}

void Vars::setMaxVarNameSize(const uint32_t &value)
{
    maxVarNameSize = value;
    iSetMaxVarNameSize();
}

uint64_t Vars::getMaxVarContentSize() const
{
    return maxVarContentSize;
}

void Vars::setMaxVarContentSize(const uint64_t &value)
{
    maxVarContentSize = value;
    iSetMaxVarContentSize();
}

void Vars::iSetMaxVarContentSize()
{
    // VIRTUAL.
}

void Vars::iSetMaxVarNameSize()
{
    // VIRTUAL.
}
