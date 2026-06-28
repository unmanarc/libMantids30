#include "json.h"

using namespace Mantids30::Helpers;

std::string JSON::toString(const json &value)
{
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::string xstrValue = Json::writeString(builder, value);

    if (!xstrValue.empty() && xstrValue[xstrValue.length() - 1] == '\n')
    {
        xstrValue.erase(xstrValue.length() - 1);
    }
    return xstrValue;
}

JSON::JSONReader2::JSONReader2()
{
    Json::CharReaderBuilder builder;
    m_reader.reset(builder.newCharReader());
}

bool JSON::JSONReader2::parse(const std::string &document, Json::Value &root)
{
    return m_reader->parse(document.c_str(), document.c_str() + document.size(), &root, &m_errors);
}

std::string JSON::JSONReader2::getFormattedErrorMessages()
{
    return m_errors;
}

std::list<std::string> JSON::toStringList(const json &value, const std::string &sub)
{
    std::list<std::string> r;

    if (sub.empty() && value.isArray())
    {
        for (size_t x = 0; x < value.size(); x++)
        {
            if (value[static_cast<int>(x)].isString())
            {
                r.push_back(value[static_cast<int>(x)].asString());
            }
        }
    }
    else if (!sub.empty() && Helpers::JSON::ISARRAY(value, sub))
    {
        for (size_t x = 0; x < value[sub].size(); x++)
        {
            if (value[sub][static_cast<int>(x)].isString())
            {
                r.push_back(value[sub][static_cast<int>(x)].asString());
            }
        }
    }
    return r;
}

std::set<std::string> JSON::toStringSet(const json &value, const std::string &sub)
{
    std::set<std::string> r;

    if (sub.empty() && value.isArray())
    {
        for (size_t x = 0; x < value.size(); x++)
        {
            if (value[static_cast<int>(x)].isString())
            {
                r.insert(value[static_cast<int>(x)].asString());
            }
        }
    }
    else if (!sub.empty() && Helpers::JSON::ISARRAY(value, sub))
    {
        for (size_t x = 0; x < value[sub].size(); x++)
        {
            if (value[sub][static_cast<int>(x)].isString())
            {
                r.insert(value[sub][static_cast<int>(x)].asString());
            }
        }
    }

    return r;
}

std::set<uint32_t> JSON::toUInt32Set(const json &value, const std::string &sub)
{
    std::set<uint32_t> r;

    if (sub.empty() && value.isArray())
    {
        for (size_t x = 0; x < value.size(); x++)
        {
            if (value[static_cast<int>(x)].isUInt())
            {
                r.insert(value[static_cast<int>(x)].asUInt());
            }
        }
    }
    else if (!sub.empty() && Helpers::JSON::ISARRAY(value, sub))
    {
        for (size_t x = 0; x < value[sub].size(); x++)
        {
            if (value[sub][static_cast<int>(x)].isUInt())
            {
                r.insert(value[sub][static_cast<int>(x)].asUInt());
            }
        }
    }

    return r;
}

json JSON::fromSet(const std::set<std::string> &t)
{
    json x;
    int v = 0;
    for (const std::string &i : t)
    {
        x[v++] = i;
    }
    return x;
}

json JSON::fromSet(const std::set<uint32_t> &t)
{
    json x;
    int v = 0;
    for (const uint32_t &i : t)
    {
        x[v++] = i;
    }
    return x;
}

json JSON::fromList(const std::list<std::string> &t)
{
    json x;
    int v = 0;
    for (const std::string &i : t)
    {
        x[v++] = i;
    }
    return x;
}

std::map<std::string, std::string> JSON::toMap(const json &jValue)
{
    std::map<std::string, std::string> r;
    for (const std::string &memberName : jValue.getMemberNames())
    {
        if (jValue[memberName].isString())
        {
            r[memberName] = Helpers::JSON::ASSTRING(jValue, memberName, "");
        }
    }
    return r;
}

Json::Value JSON::parse(const char *json)
{
    Json::Value r;
    Json::Reader().parse(json, r);
    return r;
}
