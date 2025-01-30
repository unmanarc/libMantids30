#include "vars.h"
#include <memory>

using namespace Mantids30::Memory::Abstract;

Vars::Vars()
{
    maxVarNameSize = 256; // 256 bytes
    maxVarContentSize = 128*1024; // 128Kb.
}

Vars::~Vars()
{
}

json Vars::getVarsAsJSONMap()
{
    Json::Value jsonMap;

    // Get the list of keys from the container
    std::set<std::string> keysList = getKeysList();

    for (const std::string& key : keysList)
    {
        // Get the count of variables with this key
        uint32_t count = varCount(key);

        if (count == 1)
        {
            // If there's only one value, store it directly
            std::shared_ptr<Mantids30::Memory::Streams::StreamableObject> value = getValue(key);
            jsonMap[key] = value->toString();
        }
        else if (count > 1)
        {
            // If there are multiple values, store them in an array
            std::list<std::shared_ptr<Mantids30::Memory::Streams::StreamableObject>> values = getValues(key);
            Json::Value jsonArray;

            for (std::shared_ptr<Mantids30::Memory::Streams::StreamableObject> value : values)
            {
                jsonArray.append(value->toString());
            }

            jsonMap[key] = jsonArray;
        }
    }

    return jsonMap;

}
/*
std::string Vars::getStringValue(const std::string &varName)
{
    auto * value = getValue(varName);
    return !value?"":value->toString();
}*/
/*
std::list<std::string> Vars::getStringValues(const std::string &varName)
{
    std::list<std::string> r;
    auto contList = getValues(varName);
    for (auto * b : contList)
        r.push_back(b->toString());
    return r;
}*/

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
