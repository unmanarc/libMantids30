#include "json.h"


std::string Mantids30::Helpers::jsonToString(const json &value)
{
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::string xstrValue = Json::writeString(builder, value);

    if (!xstrValue.empty() && xstrValue[xstrValue.length()-1] == '\n')
    {
        xstrValue.erase(xstrValue.length()-1);
    }
    return xstrValue;
}

Mantids30::Helpers::JSONReader2::JSONReader2()
{
    Json::CharReaderBuilder builder;
    m_reader.reset(builder.newCharReader());
}

bool Mantids30::Helpers::JSONReader2::parse(const std::string &document, Json::Value &root)
{
    return m_reader->parse(document.c_str(),document.c_str()+document.size(),&root,&m_errors);
}

std::string Mantids30::Helpers::JSONReader2::getFormattedErrorMessages()
{
    return m_errors;
}

std::list<std::string> Mantids30::Helpers::jsonToStringList(const json &value, const std::string &sub)
{
    std::list<std::string> r;

    if (sub.empty() && value.isArray())
    {
        for ( size_t x = 0; x< value.size(); x++)
        {
            if (value[static_cast<int>(x)].isString())
                r.push_back(value[static_cast<int>(x)].asString());
        }
    }
    else if (!sub.empty() && JSON_ISARRAY(value,sub))
    {
        for ( size_t x = 0; x< value[sub].size(); x++)
        {
            if (value[sub][static_cast<int>(x)].isString())
                r.push_back(value[sub][static_cast<int>(x)].asString());
        }
    }
    return r;
}

std::set<std::string> Mantids30::Helpers::jsonToStringSet(const json &value, const std::string &sub)
{
    std::set<std::string> r;

    if (sub.empty() && value.isArray())
    {
        for (size_t x = 0; x < value.size(); x++)
        {
            if (value[static_cast<int>(x)].isString())
                r.insert(value[static_cast<int>(x)].asString());
        }
    }
    else if (!sub.empty() && JSON_ISARRAY(value,sub))
    {
        for (size_t x = 0; x < value[sub].size(); x++)
        {
            if (value[sub][static_cast<int>(x)].isString())
                r.insert(value[sub][static_cast<int>(x)].asString());
        }
    }

    return r;
}

json Mantids30::Helpers::setToJSON(const std::set<std::string> &t)
{
    json x;
    int v=0;
    for (const std::string & i : t)
        x[v++] = i;
    return x;
}

json Mantids30::Helpers::setToJSON(const std::set<uint32_t> &t)
{
    json x;
    int v=0;
    for (const uint32_t & i : t)
        x[v++] = i;
    return x;
}

json Mantids30::Helpers::listToJSON(const std::list<std::string> &t)
{
    json x;
    int v = 0;
    for (const std::string &i : t)
        x[v++] = i;
    return x;
}
