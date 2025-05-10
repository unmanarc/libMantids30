#include "vars.h"
#include "b_chunks.h"
#include <memory>

using namespace Mantids30::Memory::Abstract;

Vars::Vars()
{
    m_maxVarNameSize = 256; // 256 bytes
    m_maxVarContentSize = 128*1024; // 128Kb.
}


json Vars::toJSON()
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

bool Vars::fromJSON(const Json::Value &json)
{
    clear();

    if (!json.isObject())
    {
        return false; // Not a JSON object
    }

    for (const auto &key : json.getMemberNames())
    {
        const Json::Value &value = json[key];
        if (value.isArray())
        {
            // If the value is an array, add each element as a separate variable
            for (Json::ArrayIndex i = 0; i < value.size(); ++i)
            {
                auto bChunk = std::make_shared<Memory::Containers::B_Chunks>();
                bChunk->writeString(JSON_ARRAY_ASSTRING(value, i, ""));
                addVar(key, bChunk);
            }
        }
        else
        {
            // If the value is a single element, add it as a variable
            auto bChunk = std::make_shared<Memory::Containers::B_Chunks>();
            bChunk->writeString(JSON_ASSTRING_D(value, ""));
            addVar(key, bChunk);
        }
    }

    return true;
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

std::string Vars::getStringValue(
    const std::string &varName)
{
    auto v = getValue(varName);
    if (v == nullptr)
        return "";
    return v->toString();
}

uint32_t Vars::getMaxVarNameSize() const
{
    return m_maxVarNameSize;
}

void Vars::setMaxVarNameSize(const uint32_t &value)
{
    m_maxVarNameSize = value;
    iSetMaxVarNameSize();
}

uint64_t Vars::getMaxVarContentSize() const
{
    return m_maxVarContentSize;
}

void Vars::setMaxVarContentSize(const uint64_t &value)
{
    m_maxVarContentSize = value;
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
