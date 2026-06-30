#pragma once

#include "mustache_context.h"
#include "mustache_parser.h"
#include <Mantids30/Helpers/encoders.h>
#include <functional>
#include <string>
#include <unordered_map>

namespace Mantids30::DataFormat::Mustache {

/**
 * @brief Callback for executing external functions (JFUNC equivalent).
 * @param functionName Name of the function to call
 * @param params JSON parameters
 * @return JSON result value
 */
using FunctionCallback = std::function<Json::Value(const std::string &functionName, const Json::Value &params)>;

/**
 * @brief Callback for loading included template content.
 * @param includePath Path to the template file/content to include
 * @return Template string content, or empty string if not found
 */
using IncludeCallback = std::function<std::string(const std::string &includePath)>;

/**
 * @brief Handles Mustache extension tags: js, get, post, sess, cookie, header, call, include, config.
 *
 * Extension syntax:
 * - {{>js varName := varPath}}        -> const varName = resolvedValue;
 * - {{>get varName := paramName}}     -> const varName = GET[paramName];
 * - {{>post varName := paramName}}    -> const varName = POST[paramName];
 * - {{>sess varName := key}}          -> const varName = session[key];
 * - {{>cookie varName := name}}       -> const varName = cookies[name];
 * - {{>header varName := Name}}       -> const varName = headers[Name];
 * - {{>call result := fn({})}}        -> Execute function, store result
 * - {{>include path/to/file.html}}    -> Include template
 * - {{>include! path}}                -> Include without recursion
 * - {{>include div:path}}             -> Include wrapped in <div>
 * - {{>config maxIncludeDepth := 10}} -> Set configuration
 */
class MustacheExtensions
{
public:
    MustacheExtensions() = default;
    virtual ~MustacheExtensions() = default;

    /**
     * @brief Set the callback for executing external functions.
     */
    void setFunctionCallback(FunctionCallback cb);

    /**
     * @brief Set the callback for loading included templates.
     */
    void setIncludeCallback(IncludeCallback cb);

    /**
     * @brief Set maximum include depth to prevent infinite recursion.
     * @param depth Maximum nesting depth (default: 10)
     */
    void setMaxIncludeDepth(size_t depth) { m_maxIncludeDepth = depth; }

    /**
     * @brief Get current max include depth.
     */
    size_t getMaxIncludeDepth() const { return m_maxIncludeDepth; }

    /**
     * @brief Set HTTP source data for get/post/cookie/header extensions.
     */
    void setHTTPGetVars(const std::shared_ptr<Memory::Abstract::Vars> &vars);
    void setHTTPPostVars(const std::shared_ptr<Memory::Abstract::Vars> &vars);
    void setCookies(const std::unordered_map<std::string, std::string> &cookies);
    void setHeaders(const std::unordered_map<std::string, std::string> &headers);

    /**
     * @brief Process an extension tag and return the HTML/JS output.
     * @param tag Parsed extension tag
     * @param context Variable context
     * @param currentDepth Current include depth (for recursion protection)
     * @return Rendered string output
     */
    std::string process(const ExtensionTag &tag, const MustacheContext &context,
                        size_t currentDepth = 0) const;

    /**
     * @brief Process a {{>js target := source}} tag.
     * Outputs: <script>const target = JSON.stringify(resolvedValue);</script>
     */
    std::string processJS(const ExtensionTag &tag, const MustacheContext &context) const;

    /**
     * @brief Process a {{>call target := function(params)}} tag.
     * Executes the function via callback and returns the result as JSON script tag.
     */
    std::string processCall(const ExtensionTag &tag, const MustacheContext &context,
                            size_t currentDepth) const;

    /**
     * @brief Process a {{>include path}} tag.
     * Loads and returns the included template content (further processable).
     */
    std::string processInclude(const ExtensionTag &tag, const MustacheContext &context,
                               size_t currentDepth) const;

    /**
     * @brief Process a {{>config key := value}} tag.
     * Returns empty string (configuration is applied internally).
     */
    std::string processConfig(const ExtensionTag &tag) const;

    /**
     * @brief Resolve a source from a specific HTTP source type.
     */
    std::string resolveFromSource(const std::string &sourceType, const std::string &sourceName,
                                  const MustacheContext &context) const;

    /**
     * @brief Convert a Json::Value to a JSON string for JavaScript embedding.
     */
    static std::string jsonToJSString(const Json::Value &value);

    /**
     * @brief HTML-escape a string for safe embedding.
     */
    static std::string htmlEscape(const std::string &s);

    /**
     * @brief JavaScript-escape a string for safe embedding in JS strings.
     */
    static std::string jsEscape(const std::string &s);

private:
    FunctionCallback m_functionCallback;
    IncludeCallback m_includeCallback;
    size_t m_maxIncludeDepth = 10;

    std::shared_ptr<Memory::Abstract::Vars> m_getVars;
    std::shared_ptr<Memory::Abstract::Vars> m_postVars;
    std::unordered_map<std::string, std::string> m_cookies;
    std::unordered_map<std::string, std::string> m_headers;

    /**
     * @brief Parse a function call string like "fnName({\"key\":\"val\"})" into name and params.
     */
    static std::pair<std::string, Json::Value> parseFunctionCall(const std::string &callStr);

    /**
     * @brief Generate a <script> tag with a const declaration.
     */
    static std::string makeConstScript(const std::string &varName, const std::string &jsonValue);
};

} // namespace Mantids30::DataFormat::Mustache