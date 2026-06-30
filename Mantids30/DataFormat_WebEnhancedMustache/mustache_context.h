#pragma once

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Memory/vars.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Mantids30::DataFormat::Mustache {

/**
 * @brief A single variable source with priority and resolver function.
 * Lower priority number = higher precedence in variable resolution.
 */
struct VarSource
{
    int priority;
    std::string name;
    std::function<Json::Value(const std::string &path)> resolver;
    std::function<bool(const std::string &path)> exists;
};

/**
 * @brief MustacheContext manages multiple variable sources with hierarchical resolution.
 *
 * Variables are resolved by checking sources in priority order (lowest number first).
 * Supports dot-notation paths like "user.profile.name" or "items.0.title".
 */
class MustacheContext
{
public:
    MustacheContext() = default;
    virtual ~MustacheContext() = default;

    /**
     * @brief Add custom variables with a given priority.
     * @param vars JSON object containing key-value pairs
     * @param priority Lower number = higher precedence (default 100)
     * @param sourceName Human-readable name for debugging
     */
    void addCustomVars(const Json::Value &vars, int priority = 100, const std::string &sourceName = "custom");

    /**
     * @brief Add session variables.
     * @param vars JSON object containing session data
     * @param priority Lower number = higher precedence (default 200)
     */
    void addSessionVars(const Json::Value &vars, int priority = 200);

    /**
     * @brief Add HTTP request variables (GET, POST, etc.) from a Vars object.
     * @param vars Pointer to the Memory::Abstract::Vars instance
     * @param sourceName Source identifier ("GET", "POST", etc.)
     * @param priority Lower number = higher precedence (default 300)
     */
    void addHTTPVars(const std::shared_ptr<Memory::Abstract::Vars> &vars, const std::string &sourceName, int priority = 300);

    /**
     * @brief Add network/connection info variables.
     * @param info JSON object with network data (remoteAddress, userAgent, etc.)
     * @param priority Lower number = higher precedence (default 400)
     */
    void addNetworkInfo(const Json::Value &info, int priority = 400);

    /**
     * @brief Add a variable source with a custom resolver function.
     * @param priority Precedence level
     * @param name Source name for debugging
     * @param resolver Function that returns Json::Value for a given path
     * @param exists Function that checks if a path exists in this source
     */
    void addSource(int priority, const std::string &name, 
                   std::function<Json::Value(const std::string &)> resolver,
                   std::function<bool(const std::string &)> exists);

    /**
     * @brief Resolve a variable path using dot notation.
     * @param path Dot-separated path (e.g., "user.name", "items.0.field")
     * @return Json::Value for the resolved path, or Json::nullValue if not found
     */
    [[nodiscard]] Json::Value resolve(const std::string &path) const;

    /**
     * @brief Check if a variable path exists in any source.
     * @param path Dot-separated path
     * @return true if the variable exists in at least one source
     */
    [[nodiscard]] bool exists(const std::string &path) const;

    /**
     * @brief Get all registered source names (for debugging).
     */
    [[nodiscard]] std::vector<std::string> getSourceNames() const;

    /**
     * @brief Clear all variable sources.
     */
    void clear();

    /**
     * @brief Check if the context has no sources registered.
     */
    [[nodiscard]] bool empty() const { return m_sources.empty(); }

private:
    /**
     * @brief Resolve a dot-notation path within a single Json::Value.
     * @param root The root JSON value
     * @param path Dot-separated path
     * @return Resolved value or Json::nullValue
     */
    static Json::Value resolveJsonPath(const Json::Value &root, const std::string &path);

    /**
     * @brief Check if a dot-notation path exists within a Json::Value.
     */
    static bool existsInJson(const Json::Value &root, const std::string &path);

    std::vector<VarSource> m_sources;
};

} // namespace Mantids30::DataFormat::Mustache