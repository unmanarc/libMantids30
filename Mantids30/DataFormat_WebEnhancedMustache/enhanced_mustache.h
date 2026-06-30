#pragma once

#include "mustache_context.h"
#include "mustache_parser.h"
#include "mustache_extensions.h"

#include <Mantids30/Memory/streamable_object.h>
#include <Mantids30/Memory/streamable_string.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Mantids30::DataFormat::Mustache {

/**
 * @brief Enhanced Mustache template engine with extensions.
 *
 * Supports standard Mustache syntax plus extensions:
 * - {{>js varName := varPath}}        - JavaScript const declaration
 * - {{>get varName := paramName}}     - From GET params
 * - {{>post varName := paramName}}    - From POST params
 * - {{>sess varName := key}}          - From session
 * - {{>cookie varName := name}}       - From cookies
 * - {{>header varName := Name}}       - From headers
 * - {{>call result := fn({})}}        - Execute external function
 * - {{>include path}}                 - Include template
 * - {{>include! path}}                - Include without recursion
 * - {{>include div:path}}             - Include wrapped in <div>
 * - {{>config key := value}}          - Configuration
 */
class EnhancedMustache
{
public:
    EnhancedMustache() = default;
    virtual ~EnhancedMustache() = default;

    /**
     * @brief Set the variable context for rendering.
     */
    void setContext(const MustacheContext& context);

    /**
     * @brief Get a reference to the mutable context for adding more sources.
     */
    MustacheContext &getContext();

    /**
     * @brief Set the function callback for {{>call ...}} extensions.
     */
    void setFunctionCallback(FunctionCallback cb);

    /**
     * @brief Set the include callback for {{>include ...}} extensions.
     */
    void setIncludeCallback(IncludeCallback cb);

    /**
     * @brief Set maximum include depth.
     */
    void setMaxIncludeDepth(size_t depth);

    /**
     * @brief Set HTTP GET variables.
     */
    void setHTTPGetVars(const std::shared_ptr<Memory::Abstract::Vars> &vars);

    /**
     * @brief Set HTTP POST variables.
     */
    void setHTTPPostVars(const std::shared_ptr<Memory::Abstract::Vars> &vars);

    /**
     * @brief Set cookies.
     */
    void setCookies(const std::unordered_map<std::string, std::string> &cookies);

    /**
     * @brief Set headers.
     */
    void setHeaders(const std::unordered_map<std::string, std::string> &headers);

    /**
     * @brief Render a template string.
     * @param templateStr The Mustache template
     * @return Rendered output string
     */
    std::string render(const std::string &templateStr);

    /**
     * @brief Render a template from a StreamableObject.
     * @param input Streamable object containing the template
     * @return Rendered output string
     */
    std::string renderFromStream(Memory::Streams::StreamableObject &input);

    /**
     * @brief Render a template and write the output to a StreamableObject.
     * @param templateStr The Mustache template
     * @param output Output streamable object
     */
    void renderToStream(const std::string &templateStr, Memory::Streams::StreamableObject &output);

    /**
     * @brief Render a template from input stream to output stream.
     */
    void renderStreamToStream(Memory::Streams::StreamableObject &input,
                               Memory::Streams::StreamableObject &output);

    /**
     * @brief Check if the engine is ready (has context configured).
     */
    bool isReady() const;

    /**
     * @brief Reset the engine state.
     */
    void reset();

private:
    MustacheContext m_context;
    MustacheParser m_parser;
    MustacheExtensions m_extensions;
    size_t m_currentDepth = 0;

    /**
     * @brief Process tokens and produce the rendered output.
     */
    std::string processTokens(const std::vector<Token> &tokens, size_t depth);

    /**
     * @brief Render a section's content with a specific value context.
     */
    std::string renderSection(const std::string &name, const std::vector<Token> &bodyTokens,
                               const Json::Value &value, size_t depth);

    /**
     * @brief Resolve a variable's string representation.
     */
    std::string resolveVariable(const std::string &name) const;

    /**
     * @brief Convert a Json::Value to its string representation for template output.
     */
    static std::string jsonToString(const Json::Value &value);

    /**
     * @brief Extract body tokens between a section open and close.
     */
    static std::vector<Token> extractSectionBody(const std::vector<Token> &tokens,
                                                  size_t openIndex, const std::string &sectionName);

    /**
     * @brief Check if a value is "falsy" for inverted sections.
     */
    static bool isFalsy(const Json::Value &value);
};

} // namespace Mantids30::DataFormat::Mustache