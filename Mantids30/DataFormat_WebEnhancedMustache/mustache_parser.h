#pragma once

#include <Mantids30/Memory/streamable_object.h>
#include <Mantids30/Memory/streamable_string.h>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace Mantids30::DataFormat::Mustache {

/**
 * @brief Types of tokens in the Mustache template.
 */
enum class TokenType : uint8_t
{
    TEXT,               // Plain text content
    VARIABLE,           // {{var}} or {{var.path}}
    SECTION_OPEN,       // {{#section}}
    SECTION_CLOSE,      // {{/section}}
    INVERTED_SECTION,   // {{^inverted}}
    PARTIAL,            // {{>partial}} (standard Mustache partial)
    RAW_BLOCK,          // {{{{raw}}}}
    COMMENT,            // {{!comment}}
    EXTENSION,          // {{>js ...}}, {{>call ...}}, {{>include ...}}, etc.
};

/**
 * @brief A parsed token from the template.
 */
struct Token
{
    TokenType type;
    std::string raw;         // Original text (including {{ }})
    std::string content;     // Inner content (without {{ }})
    size_t position;         // Start position in original template
    int nestingLevel;        // Nesting depth for sections
};

/**
 * @brief Parsed representation of an extension tag.
 * Example: {{>js varName := user.data}} -> type="js", target="varName", source="user.data"
 * Example: {{>call result := loginMode({})}} -> type="call", target="result", source="loginMode({})"
 * Example: {{>get jsName := param}} -> type="get", target="jsName", source="param"
 * Example: {{>include path/to/file.html}} -> type="include", source="path/to/file.html"
 */
struct ExtensionTag
{
    std::string type;        // "js", "get", "post", "sess", "cookie", "header", "call", "include", "config"
    std::string target;      // Left side of := (output variable name)
    std::string source;      // Right side of := (source path or function call)
    bool noRecursion;        // For include: true if {{>include! ...}}
    std::string wrapper;     // For include: tag wrapper like "div" in {{>include div:path}}
};

/**
 * @brief Mustache template parser.
 *
 * Parses a Mustache template string into a list of tokens, supporting:
 * - Variables: {{name}}
 * - Sections: {{#items}}...{{/items}}
 * - Inverted sections: {{^optional}}...{{/optional}}
 * - Standard partials: {{>partial}}
 * - Raw blocks: {{{{raw_html}}}}
 * - Comments: {{!ignored}}
 * - Extensions: {{>js x := y}}, {{>call f := fn({})}}, {{>include path}}
 */
class MustacheParser
{
public:
    MustacheParser() = default;
    virtual ~MustacheParser() = default;

    /**
     * @brief Parse a template string into tokens.
     * @param templateStr The raw template string
     * @return Vector of parsed tokens
     */
    std::vector<Token> parse(const std::string &templateStr);

    /**
     * @brief Parse a template from a StreamableObject.
     * @param input Streamable object containing the template
     * @return Vector of parsed tokens
     */
    std::vector<Token> parseFromStream(Memory::Streams::StreamableObject &input);

    /**
     * @brief Parse an extension tag string into structured data.
     * @param content The inner content of {{>...}}
     * @return Parsed extension tag
     */
    static ExtensionTag parseExtensionTag(const std::string &content);

    /**
     * @brief Check if a tag is an extension tag (starts with a keyword after >).
     */
    static bool isExtensionTag(const std::string &content);

    /**
     * @brief Get the set of known extension keywords.
     */
    static const std::unordered_set<std::string> &getExtensionKeywords();

private:
    std::vector<Token> m_tokens;

    /**
     * @brief Find the matching closing delimiter.
     * @param str Input string
     * @param start Starting position to search from
     * @return Pair of (opening_end, closing_start) positions, or (-1,-1) if not found
     */
    [[nodiscard]] std::pair<int, int> findDelimiter(const std::string &str, size_t start) const;

    /**
     * @brief Trim whitespace from both ends of a string.
     */
    static std::string trim(const std::string &s);

    /**
     * @brief Parse tokens recursively to handle nesting.
     */
    void parseRecursive(const std::string &templateStr, std::vector<Token> &tokens, 
                        size_t startPos, int nestingLevel);
};

} // namespace Mantids30::DataFormat::Mustache