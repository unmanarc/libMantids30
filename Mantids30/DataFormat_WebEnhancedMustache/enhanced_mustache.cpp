#include "enhanced_mustache.h"
#include <algorithm>

namespace Mantids30::DataFormat::Mustache {

void EnhancedMustache::setContext(const MustacheContext &context)
{
    m_context = context;
}

MustacheContext &EnhancedMustache::getContext()
{
    return m_context;
}

void EnhancedMustache::setFunctionCallback(FunctionCallback cb)
{
    m_extensions.setFunctionCallback(std::move(cb));
}

void EnhancedMustache::setIncludeCallback(IncludeCallback cb)
{
    m_extensions.setIncludeCallback(std::move(cb));
}

void EnhancedMustache::setMaxIncludeDepth(size_t depth)
{
    m_extensions.setMaxIncludeDepth(depth);
}

void EnhancedMustache::setHTTPGetVars(const std::shared_ptr<Memory::Abstract::Vars> &vars)
{
    m_extensions.setHTTPGetVars(vars);
}

void EnhancedMustache::setHTTPPostVars(const std::shared_ptr<Memory::Abstract::Vars> &vars)
{
    m_extensions.setHTTPPostVars(vars);
}

void EnhancedMustache::setCookies(const std::unordered_map<std::string, std::string> &cookies)
{
    m_extensions.setCookies(cookies);
}

void EnhancedMustache::setHeaders(const std::unordered_map<std::string, std::string> &headers)
{
    m_extensions.setHeaders(headers);
}

std::string EnhancedMustache::render(const std::string &templateStr)
{
    auto tokens = m_parser.parse(templateStr);
    return processTokens(tokens, 0);
}

std::string EnhancedMustache::renderFromStream(Memory::Streams::StreamableObject &input)
{
    auto tokens = m_parser.parseFromStream(input);
    return processTokens(tokens, 0);
}

void EnhancedMustache::renderToStream(const std::string &templateStr, Memory::Streams::StreamableObject &output)
{
    Memory::Streams::StreamableString strObj;
    *(strObj.getDirectMemory()) = render(templateStr);
    strObj.streamTo(&output);
}

void EnhancedMustache::renderStreamToStream(Memory::Streams::StreamableObject &input, Memory::Streams::StreamableObject &output)
{
    Memory::Streams::StreamableString strObj;
    *(strObj.getDirectMemory()) = renderFromStream(input);
    strObj.streamTo(&output);
}

bool EnhancedMustache::isReady() const
{
    return !m_context.empty();
}

void EnhancedMustache::reset()
{
    m_context.clear();
    m_currentDepth = 0;
}

std::string EnhancedMustache::processTokens(const std::vector<Token> &tokens, size_t depth)
{
    std::string result;
    result.reserve(tokens.size() * 20); // Rough estimate

    for (size_t i = 0; i < tokens.size(); i++)
    {
        const auto &token = tokens[i];

        switch (token.type)
        {
        case TokenType::TEXT:
            result += token.content;
            break;

        case TokenType::COMMENT:
            // Comments are silently ignored
            break;

        case TokenType::VARIABLE:
            result += resolveVariable(token.content);
            break;

        case TokenType::SECTION_OPEN:
        {
            auto body = extractSectionBody(tokens, i, token.content);
            Json::Value value = m_context.resolve(token.content);

            if (value.isArray())
            {
                // Iterate over array
                std::string sectionResult;
                for (const auto &item : value)
                {
                    sectionResult += renderSection(token.content, body, item, depth);
                }
                result += sectionResult;
            }
            else if (!value.isNull() && value != Json::Value::nullSingleton())
            {
                // Single value or object - render once with scoped context
                result += renderSection(token.content, body, value, depth);
            }
            // If null/falsy, skip the section (output nothing)

            // Skip past the section body and closing tag
            i += body.size() + 1;
            break;
        }

        case TokenType::INVERTED_SECTION:
        {
            auto body = extractSectionBody(tokens, i, token.content);
            Json::Value value = m_context.resolve(token.content);

            if (isFalsy(value))
            {
                result += processTokens(body, depth);
            }

            i += body.size() + 1;
            break;
        }

        case TokenType::SECTION_CLOSE:
            // Handled by section processing
            break;

        case TokenType::PARTIAL:
        {
            // Standard Mustache partial - use include callback
            if (m_extensions.getMaxIncludeDepth() > 0 && depth < m_extensions.getMaxIncludeDepth())
            {
                ExtensionTag tag;
                tag.type = "include";
                tag.source = token.content;
                tag.noRecursion = false;

                std::string partialContent = m_extensions.processInclude(tag, m_context, depth);
                if (!partialContent.empty() && partialContent.find("<!--") != 0)
                {
                    // Recursively process the included content
                    auto partialTokens = m_parser.parse(partialContent);
                    result += processTokens(partialTokens, depth + 1);
                }
                else if (partialContent.empty() || partialContent.find("<!--") == 0)
                {
                    result += partialContent;
                }
            }
            break;
        }

        case TokenType::RAW_BLOCK:
            // Raw blocks output the variable value without escaping
            result += resolveVariable(token.content);
            break;

        case TokenType::EXTENSION:
        {
            ExtensionTag tag = Mantids30::DataFormat::Mustache::MustacheParser::parseExtensionTag(token.content);

            // Handle config extensions specially
            if (tag.type == "config")
            {
                if (tag.target == "maxIncludeDepth")
                {
                    try
                    {
                        m_extensions.setMaxIncludeDepth(std::stoul(tag.source));
                    }
                    catch (...)
                    {}
                }
            }
            else
            {
                std::string extResult = m_extensions.process(tag, m_context, depth);

                // For include extensions that return template content, process recursively
                if (tag.type == "include" && !extResult.empty() && !tag.noRecursion)
                {
                    if (depth < m_extensions.getMaxIncludeDepth())
                    {
                        auto includedTokens = m_parser.parse(extResult);
                        result += processTokens(includedTokens, depth + 1);
                    }
                }
                else
                {
                    result += extResult;
                }
            }
            break;
        }
        }
    }

    return result;
}

std::string EnhancedMustache::renderSection(const std::string &name, const std::vector<Token> &bodyTokens, const Json::Value &value, size_t depth)
{
    // Create a scoped context that adds the current section value
    MustacheContext scopedContext = m_context;

    // Add the section value at the section name path
    scopedContext.addCustomVars(value, 0, "section:" + name);

    // Process body with the scoped context
    std::string result;
    result.reserve(bodyTokens.size() * 20);

    for (const auto &token : bodyTokens)
    {
        switch (token.type)
        {
        case TokenType::TEXT:
            result += token.content;
            break;

        case TokenType::COMMENT:
            break;

        case TokenType::VARIABLE:
            // Try scoped context first, then fall back to main context
            {
                Json::Value scopedVal = scopedContext.resolve(token.content);
                if (scopedVal.isNull())
                {
                    scopedVal = m_context.resolve(token.content);
                }
                result += jsonToString(scopedVal);
                break;
            }

        case TokenType::SECTION_OPEN:
        {
            auto body = extractSectionBody(bodyTokens, std::find_if(bodyTokens.begin(), bodyTokens.end(), [&token](const Token &t) { return t.position == token.position; }) - bodyTokens.begin(),
                                           token.content);
            Json::Value scopedVal = scopedContext.resolve(token.content);
            if (scopedVal.isNull())
            {
                scopedVal = m_context.resolve(token.content);
            }

            if (scopedVal.isArray())
            {
                for (const auto &item : scopedVal)
                {
                    result += renderSection(token.content, body, item, depth);
                }
            }
            else if (!scopedVal.isNull())
            {
                result += renderSection(token.content, body, scopedVal, depth);
            }
            break;
        }

        case TokenType::INVERTED_SECTION:
        {
            auto body = extractSectionBody(bodyTokens, std::find_if(bodyTokens.begin(), bodyTokens.end(), [&token](const Token &t) { return t.position == token.position; }) - bodyTokens.begin(),
                                           token.content);
            Json::Value val = scopedContext.resolve(token.content);
            if (val.isNull())
            {
                val = m_context.resolve(token.content);
            }

            if (isFalsy(val))
            {
                result += processTokens(body, depth);
            }
            break;
        }

        case TokenType::PARTIAL:
        {
            ExtensionTag tag;
            tag.type = "include";
            tag.source = token.content;
            std::string partialContent = m_extensions.processInclude(tag, scopedContext, depth);
            if (!partialContent.empty() && partialContent.find("<!--") != 0)
            {
                auto partialTokens = m_parser.parse(partialContent);
                // Temporarily swap context for partial processing
                MustacheContext saved = m_context;
                m_context = scopedContext;
                result += processTokens(partialTokens, depth + 1);
                m_context = saved;
            }
            else
            {
                result += partialContent;
            }
            break;
        }

        case TokenType::EXTENSION:
        {
            ExtensionTag tag = Mantids30::DataFormat::Mustache::MustacheParser::parseExtensionTag(token.content);
            std::string extResult = m_extensions.process(tag, scopedContext, depth);

            if (tag.type == "include" && !extResult.empty() && !tag.noRecursion)
            {
                if (depth < m_extensions.getMaxIncludeDepth())
                {
                    MustacheContext saved = m_context;
                    m_context = scopedContext;
                    auto includedTokens = m_parser.parse(extResult);
                    result += processTokens(includedTokens, depth + 1);
                    m_context = saved;
                }
            }
            else
            {
                result += extResult;
            }
            break;
        }

        default:
            break;
        }
    }

    return result;
}

std::string EnhancedMustache::resolveVariable(const std::string &name) const
{
    Json::Value value = m_context.resolve(name);
    return jsonToString(value);
}

std::string EnhancedMustache::jsonToString(const Json::Value &value)
{
    if (value.isNull())
    {
        return "";
    }
    if (value.isBool())
    {
        return value.asBool() ? "true" : "false";
    }
    if (value.isInt())
    {
        return std::to_string(value.asInt64());
    }
    if (value.isUInt())
    {
        return std::to_string(value.asUInt64());
    }
    if (value.isDouble() || value.isIntegral())
    {
        return std::to_string(value.asDouble());
    }
    if (value.isString())
    {
        return value.asString();
    }
    if (value.isArray() || value.isObject())
    {
        Json::StreamWriterBuilder builder;
        builder["commentStyle"] = "None";
        builder["indentation"] = "";
        return Json::writeString(builder, value);
    }
    return "";
}

std::vector<Token> EnhancedMustache::extractSectionBody(const std::vector<Token> &tokens, size_t openIndex, const std::string &sectionName)
{
    std::vector<Token> body;
    int nesting = 1;

    for (size_t i = openIndex + 1; i < tokens.size(); i++)
    {
        const auto &token = tokens[i];

        if (token.type == TokenType::SECTION_OPEN || token.type == TokenType::INVERTED_SECTION)
        {
            nesting++;
            body.push_back(token);
        }
        else if (token.type == TokenType::SECTION_CLOSE)
        {
            nesting--;
            if (nesting == 0)
            {
                // Found the matching close tag
                break;
            }
            body.push_back(token);
        }
        else
        {
            body.push_back(token);
        }
    }

    return body;
}

bool EnhancedMustache::isFalsy(const Json::Value &value)
{
    return value.isNull() || (value.isBool() && !value.asBool()) || (value.isArray() && value.empty()) || (value.isString() && value.asString().empty());
}

} // namespace Mantids30::DataFormat::Mustache