#include "mustache_parser.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <regex>

namespace Mantids30::DataFormat::Mustache {

namespace {

const std::unordered_set<std::string> EXTENSION_KEYWORDS = {"js", "get", "post", "sess", "cookie", "header", "call", "include", "config"};

} // anonymous namespace

const std::unordered_set<std::string> &MustacheParser::getExtensionKeywords()
{
    return EXTENSION_KEYWORDS;
}

std::string MustacheParser::trim(const std::string &s)
{
    return boost::algorithm::trim_copy(s);
}

std::pair<int, int> MustacheParser::findDelimiter(const std::string &str, size_t start) const
{
    auto remaining = str.substr(start);

    // Check for {{{{ (raw block)
    if (boost::algorithm::starts_with(remaining, "{{{{"))
    {
        size_t closePos = str.find("}}}}", start + 4);
        if (closePos != std::string::npos)
        {
            return {static_cast<int>(start + 4), static_cast<int>(closePos)};
        }
        return {-1, -1};
    }

    // Check for {{
    if (boost::algorithm::starts_with(remaining, "{{"))
    {
        size_t closePos = str.find("}}", start + 2);
        if (closePos != std::string::npos)
        {
            return {static_cast<int>(start + 2), static_cast<int>(closePos)};
        }
        return {-1, -1};
    }

    return {-1, -1};
}

bool MustacheParser::isExtensionTag(const std::string &content)
{
    static const std::regex re{R"(>\s*(js|get|post|sess|cookie|header|call|include|config)(\!)?)"};
    return std::regex_search(content, re);
}

ExtensionTag MustacheParser::parseExtensionTag(const std::string &content)
{
    static const std::regex re{R"(>\s*(js|get|post|sess|cookie|header|call|include|config)(\!)?\s*(.*))"};
    std::smatch m;
    ExtensionTag tag;
    tag.noRecursion = false;

    if (!std::regex_search(content, m, re))
    {
        return tag;
    }

    tag.type = m[1].str();
    tag.noRecursion = m[2].matched;
    std::string args = boost::algorithm::trim_copy(m[3].str());

    if (tag.type == "include")
    {
        // Format: [wrapper:]path or just path
        size_t colonPos = args.find(':');
        if (colonPos != std::string::npos && colonPos < args.find('/'))
        {
            tag.wrapper = boost::algorithm::trim_copy(args.substr(0, colonPos));
            tag.source = boost::algorithm::trim_copy(args.substr(colonPos + 1));
        }
        else
        {
            tag.source = args;
        }
    }
    else if (tag.type == "config")
    {
        // Format: key := value
        size_t assignPos = args.find(":=");
        if (assignPos != std::string::npos)
        {
            tag.target = boost::algorithm::trim_copy(args.substr(0, assignPos));
            tag.source = boost::algorithm::trim_copy(args.substr(assignPos + 2));
        }
        else
        {
            tag.source = args;
        }
    }
    else
    {
        // Format: target := source (for js, get, post, sess, cookie, header, call)
        size_t assignPos = args.find(":=");
        if (assignPos != std::string::npos)
        {
            tag.target = boost::algorithm::trim_copy(args.substr(0, assignPos));
            tag.source = boost::algorithm::trim_copy(args.substr(assignPos + 2));
        }
        else
        {
            tag.source = args;
        }
    }

    return tag;
}

void MustacheParser::parseRecursive(const std::string &templateStr, std::vector<Token> &tokens, size_t startPos, int nestingLevel)
{
    size_t pos = startPos;
    size_t textStart = startPos;

    while (pos < templateStr.size())
    {
        auto [openEnd, closeStart] = findDelimiter(templateStr, pos);
        if (openEnd == -1)
        {
            // No more delimiters, rest is text
            if (pos > textStart)
            {
                Token textToken;
                textToken.type = TokenType::TEXT;
                textToken.content = templateStr.substr(textStart, pos - textStart);
                textToken.position = textStart;
                textToken.nestingLevel = nestingLevel;
                tokens.push_back(textToken);
            }
            break;
        }

        // Add text before this delimiter
        if (openEnd > static_cast<int>(pos))
        {
            Token textToken;
            textToken.type = TokenType::TEXT;
            textToken.content = templateStr.substr(pos, openEnd - pos);
            textToken.position = pos;
            textToken.nestingLevel = nestingLevel;
            tokens.push_back(textToken);
        }

        // Extract content between delimiters
        std::string raw = templateStr.substr(pos, closeStart + 2 - pos);
        std::string content = templateStr.substr(openEnd, closeStart - openEnd);
        std::string trimmedContent = trim(content);

        Token token;
        token.raw = raw;
        token.content = trimmedContent;
        token.position = pos;
        token.nestingLevel = nestingLevel;

        // Determine token type
        if (trimmedContent.empty())
        {
            token.type = TokenType::TEXT;
            token.content = raw;
        }
        else if (!content.empty() && content[0] == '!')
        {
            token.type = TokenType::COMMENT;
        }
        else if (!content.empty() && content[0] == '#')
        {
            token.type = TokenType::SECTION_OPEN;
            token.content = trim(content.substr(1));
        }
        else if (!content.empty() && content[0] == '/')
        {
            token.type = TokenType::SECTION_CLOSE;
            token.content = trim(content.substr(1));
        }
        else if (!content.empty() && content[0] == '^')
        {
            token.type = TokenType::INVERTED_SECTION;
            token.content = trim(content.substr(1));
        }
        else if (!content.empty() && content[0] == '>')
        {
            std::string afterArrow = trim(content.substr(1));
            if (isExtensionTag(content))
            {
                token.type = TokenType::EXTENSION;
            }
            else
            {
                token.type = TokenType::PARTIAL;
                token.content = afterArrow;
            }
        }
        else
        {
            token.type = TokenType::VARIABLE;
        }

        tokens.push_back(token);

        // Move position past this tag
        pos = closeStart + 2;
        textStart = pos;
    }
}

std::vector<Token> MustacheParser::parse(const std::string &templateStr)
{
    m_tokens.clear();
    parseRecursive(templateStr, m_tokens, 0, 0);
    return m_tokens;
}

std::vector<Token> MustacheParser::parseFromStream(Memory::Streams::StreamableObject &input)
{
    Memory::Streams::StreamableString strObj;
    input.streamTo(&strObj);
    return parse(strObj.getValue());
}

} // namespace Mantids30::DataFormat::Mustache