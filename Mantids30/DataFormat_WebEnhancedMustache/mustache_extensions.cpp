#include "mustache_extensions.h"

#include <Mantids30/Helpers/encoders.h>

#include <boost/algorithm/string.hpp>
#include <regex>
#include <sstream>

namespace Mantids30::DataFormat::Mustache {

void MustacheExtensions::setFunctionCallback(FunctionCallback cb)
{
    m_functionCallback = std::move(cb);
}

void MustacheExtensions::setIncludeCallback(IncludeCallback cb)
{
    m_includeCallback = std::move(cb);
}

void MustacheExtensions::setHTTPGetVars(const std::shared_ptr<Memory::Abstract::Vars> &vars)
{
    m_getVars = vars;
}

void MustacheExtensions::setHTTPPostVars(const std::shared_ptr<Memory::Abstract::Vars> &vars)
{
    m_postVars = vars;
}

void MustacheExtensions::setCookies(const std::unordered_map<std::string, std::string> &cookies)
{
    m_cookies = cookies;
}

void MustacheExtensions::setHeaders(const std::unordered_map<std::string, std::string> &headers)
{
    m_headers = headers;
}

std::string MustacheExtensions::process(const ExtensionTag &tag, const MustacheContext &context, size_t currentDepth) const
{
    if (tag.type.empty())
    {
        return "";
    }

    if (tag.type == "js")
    {
        return processJS(tag, context);
    }
    else if (tag.type == "call")
    {
        return processCall(tag, context, currentDepth);
    }
    else if (tag.type == "include")
    {
        return processInclude(tag, context, currentDepth);
    }
    else if (tag.type == "config")
    {
        return processConfig(tag);
    }
    else if (tag.type == "get" || tag.type == "post" || tag.type == "sess" || tag.type == "cookie" || tag.type == "header")
    {
        return resolveFromSource(tag.type, tag.source, context);
    }

    return "";
}

std::string MustacheExtensions::processJS(const ExtensionTag &tag, const MustacheContext &context) const
{
    if (tag.target.empty() || tag.source.empty())
    {
        return "";
    }

    Json::Value value = context.resolve(tag.source);
    std::string jsonStr = jsonToJSString(value);

    return makeConstScript(tag.target, jsonStr);
}

std::string MustacheExtensions::processCall(const ExtensionTag &tag, const MustacheContext &context, size_t currentDepth) const
{
    if (!m_functionCallback || tag.source.empty())
    {
        return "";
    }

    auto [funcName, params] = parseFunctionCall(tag.source);
    if (funcName.empty())
    {
        return "";
    }

    Json::Value result = m_functionCallback(funcName, params);

    if (tag.target.empty())
    {
        // No target variable, just return the result as raw JSON
        return jsonToJSString(result);
    }

    std::string jsonStr = jsonToJSString(result);
    return makeConstScript(tag.target, jsonStr);
}

std::string MustacheExtensions::processInclude(const ExtensionTag &tag, const MustacheContext &context, size_t currentDepth) const
{
    if (tag.source.empty())
    {
        return "";
    }

    // Check recursion depth
    if (currentDepth >= m_maxIncludeDepth)
    {
        return "<!-- Max include depth reached -->";
    }

    // Load content
    std::string content;
    if (m_includeCallback)
    {
        content = m_includeCallback(tag.source);
    }
    else
    {
        return "<!-- Include callback not set -->";
    }

    if (content.empty())
    {
        return "<!-- Include not found: " + htmlEscape(tag.source) + " -->";
    }

    // If noRecursion is true, just return content without further processing
    if (tag.noRecursion)
    {
        return content;
    }

    // Wrap if specified
    if (!tag.wrapper.empty())
    {
        content = "<" + tag.wrapper + ">" + content + "</" + tag.wrapper + ">";
    }

    return content;
}

std::string MustacheExtensions::processConfig(const ExtensionTag &tag) const
{
    // Config tags are processed during rendering, not here.
    // Return empty string since config doesn't produce output.
    return "";
}

std::string MustacheExtensions::resolveFromSource(const std::string &sourceType, const std::string &sourceName, const MustacheContext &context) const
{
    std::string value;

    if (sourceType == "get")
    {
        if (m_getVars && m_getVars->exist(sourceName))
        {
            auto val = m_getVars->getValue(sourceName);
            if (val)
            {
                value = val->toString();
            }
        }
    }
    else if (sourceType == "post")
    {
        if (m_postVars && m_postVars->exist(sourceName))
        {
            auto val = m_postVars->getValue(sourceName);
            if (val)
            {
                value = val->toString();
            }
        }
    }
    else if (sourceType == "cookie")
    {
        auto it = m_cookies.find(sourceName);
        if (it != m_cookies.end())
        {
            value = it->second;
        }
    }
    else if (sourceType == "header")
    {
        auto it = m_headers.find(sourceName);
        if (it != m_headers.end())
        {
            value = it->second;
        }
    }
    else if (sourceType == "sess")
    {
        Json::Value val = context.resolve(sourceName);
        if (!val.isNull())
        {
            value = val.asString();
        }
    }

    return value;
}

std::string MustacheExtensions::jsonToJSString(const Json::Value &value)
{
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

std::string MustacheExtensions::htmlEscape(const std::string &s)
{
    auto r = boost::replace_all_copy(s, std::string(1, '&'), std::string{"&amp;"});
    r = boost::replace_all_copy(r, std::string(1, '<'), std::string{"&lt;"});
    r = boost::replace_all_copy(r, std::string(1, '>'), std::string{"&gt;"});
    r = boost::replace_all_copy(r, std::string(1, '"'), std::string{"&quot;"});
    return boost::replace_all_copy(r, std::string(1, '\''), std::string{"&#39;"});
}

std::string MustacheExtensions::jsEscape(const std::string &s)
{
    auto r = boost::replace_all_copy(s, "\\", "\\\\");
    r = boost::replace_all_copy(r, "\"", "\\\"");
    r = boost::replace_all_copy(r, "'", "\\'");
    r = boost::replace_all_copy(r, "\n", "\\n");
    r = boost::replace_all_copy(r, "\r", "\\r");
    return boost::replace_all_copy(r, "\t", "\\t");
}

std::pair<std::string, Json::Value> MustacheExtensions::parseFunctionCall(const std::string &callStr)
{
    static const std::regex re{R"delim(^(.*?)\((.*)\)\s*$)delim"};
    std::smatch m;
    Json::Value params = Json::Value::nullSingleton();

    if (std::regex_match(callStr, m, re) && !m[2].str().empty())
    {
        std::istringstream iss(m[2].str());
        Json::CharReaderBuilder builder;
        std::string err;
        Json::parseFromStream(builder, iss, &params, &err);
        return {m[1].str(), params};
    }
    return {callStr, params};
}

std::string MustacheExtensions::makeConstScript(const std::string &varName, const std::string &jsonValue)
{
    return "<script>const " + varName + " = " + jsonValue + ";</script>";
}

} // namespace Mantids30::DataFormat::Mustache