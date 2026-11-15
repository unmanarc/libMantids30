#include "json.h"

#include <boost/json.hpp>

using namespace Mantids30::DataFormats::JSON;

std::string Mantids30::DataFormats::JSON::toString(const boost::json::value &value)
{
    return boost::json::serialize(value);
}

bool Mantids30::DataFormats::JSON::parse(std::string_view document, boost::json::value &root, std::string *parseErrors)
{
    try
    {
        boost::json::parser parser;
        parser.write_some(document);
        root = parser.release();

        if (parseErrors)
        {
            parseErrors->clear();
        }

        return true;
    }
    catch (const std::exception &e)
    {
        if (parseErrors)
        {
            *parseErrors = e.what();
        }

        return false;
    }
}

bool Mantids30::DataFormats::JSON::Reader::parse(std::string_view document, boost::json::value &root)
{
    // Call the free function (namespace scope) explicitly. An unqualified
    // parse() here resolves to this member, which would recurse infinitely.
    std::string err;
    if (Mantids30::DataFormats::JSON::parse(document, root, &err))
    {
        m_errors.clear();
        return true;
    }
    m_errors = err.empty() ? std::string("JSON parse error") : err;
    return false;
}

std::string Mantids30::DataFormats::JSON::Reader::getFormattedErrorMessages()
{
    return m_errors;
}

std::list<std::string> Mantids30::DataFormats::JSON::toStringList(const boost::json::value &value, std::string_view sub)
{
    std::list<std::string> r;

    const boost::json::value &arr = sub.empty() ? value : MEMBER_OR_NULL(value, sub);

    if (arr.is_array())
    {
        for (const boost::json::value &el : arr.as_array())
        {
            if (el.is_string())
            {
                r.emplace_back(el.as_string());
            }
        }
    }
    return r;
}

std::set<std::string> Mantids30::DataFormats::JSON::toStringSet(const boost::json::value &value, std::string_view sub)
{
    std::set<std::string> r;

    const boost::json::value &arr = sub.empty() ? value : MEMBER_OR_NULL(value, sub);

    if (arr.is_array())
    {
        for (const boost::json::value &el : arr.as_array())
        {
            if (el.is_string())
            {
                r.insert(std::string(el.as_string().c_str()));
            }
        }
    }

    return r;
}

std::set<uint32_t> Mantids30::DataFormats::JSON::toUInt32Set(const boost::json::value &value, std::string_view sub)
{
    std::set<uint32_t> r;

    const boost::json::value &arr = sub.empty() ? value : MEMBER_OR_NULL(value, sub);

    if (arr.is_array())
    {
        for (const boost::json::value &el : arr.as_array())
        {
            if (el.is_number())
            {
                r.insert(static_cast<uint32_t>(ASUINT64_OR_ZERO(el)));
            }
        }
    }

    return r;
}

boost::json::value Mantids30::DataFormats::JSON::fromSet(const std::set<std::string> &t)
{
    boost::json::array x;
    for (const std::string &i : t)
    {
        x.emplace_back(i);
    }
    return x;
}

boost::json::value Mantids30::DataFormats::JSON::fromSet(const std::set<uint32_t> &t)
{
    boost::json::array x;
    for (const uint32_t &i : t)
    {
        x.emplace_back(i);
    }
    return x;
}

boost::json::value Mantids30::DataFormats::JSON::fromList(const std::list<std::string> &t)
{
    boost::json::array x;
    for (const std::string &i : t)
    {
        x.emplace_back(i);
    }
    return x;
}

std::map<std::string, std::string> Mantids30::DataFormats::JSON::toMap(const boost::json::value &jValue)
{
    std::map<std::string, std::string> r;
    if (jValue.is_object())
    {
        for (const auto &kv : jValue.as_object())
        {
            if (kv.value().is_string())
            {
                r[kv.key()] = std::string(kv.value().as_string().c_str());
            }
        }
    }
    return r;
}

boost::json::value Mantids30::DataFormats::JSON::parse(const char *json)
{
    boost::json::value r;
    if (json != nullptr)
    {
        try
        {
            r = boost::json::parse(json);
        }
        catch (const std::exception &)
        {
            r = boost::json::value{};
        }
    }
    return r;
}
