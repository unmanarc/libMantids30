#pragma once

#include <Mantids30/DataFormats/JSON/json.h>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/regex.hpp>


#include <cstdint>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Mantids30::Servers::RESTfulWebServer {

class ResourcesFilter
{
public:
    ResourcesFilter() = default;

    enum class ProcessingMode : uint8_t
    {
        RAW,
        HTMLIENGINE,
        MANTIDSLANG
    };

    enum class ActionType : uint8_t
    {
        ADD_HEADERS,
        REPLACE_HEADERS,
        PROCESS_AS,
        REDIRECT,
        ACCEPT,
        DENY
    };

    struct Action
    {
        ActionType type = ActionType::ACCEPT;

        std::vector<std::pair<std::string, std::string>> httpHeaders;

        ProcessingMode processingMode = ProcessingMode::RAW;

        std::string redirectLocation;

        uint16_t statusCode = 0;
    };

    struct FilterEvaluationResult
    {
        bool matched = false;

        std::vector<Action> actions;

        [[nodiscard]] boost::json::object toJSON() const;
    };

    struct Filter
    {
        void compileRegex();

        std::list<std::string> uriRegexs;
        std::list<boost::regex> regexPatterns;

        std::list<std::string> requiredScopes;
        std::list<std::string> rejectedScopes;

        std::list<std::string> requiredRoles;
        std::list<std::string> rejectedRoles;

        bool requireSession = false;
        bool disallowSession = false;

        std::vector<Action> actions;
    };

    bool loadFiltersFromFile(const std::string &filePath);

    void addFilter(const Filter &filter);

    void clearFilters();

    [[nodiscard]] FilterEvaluationResult evaluateURI(const std::string &uri, const std::set<std::string> &scopes, const std::set<std::string> &roles, bool isSessionActive, bool isAdmin) const;

protected:
    std::list<Filter> m_filters;

private:
    static std::string normalizeIdentifier(std::string value);
    static ProcessingMode parseProcessingMode(const std::string &value);
    static uint16_t parseStatusCode(const boost::property_tree::ptree &node, uint16_t defaultStatusCode);
    static void loadStringList(const boost::property_tree::ptree &node, const std::string &childName, std::list<std::string> &destination);
    static void loadHeaders(const boost::property_tree::ptree &node, std::vector<std::pair<std::string, std::string>> &headers);
    static Action parseAction(const boost::property_tree::ptree &actionNode);
    static bool containsAll(const std::set<std::string> &values, const std::list<std::string> &requiredValues);
    static bool containsNone(const std::set<std::string> &values, const std::list<std::string> &rejectedValues);
    static bool filterRequirementsMatch(const Filter &filter, const std::set<std::string> &scopes, const std::set<std::string> &roles, bool isSessionActive, bool isAdmin);
    static bool isTerminalAction(ActionType type);
    static bool filterURIMatches(const Filter &filter, const std::string &uri);
    static std::string actionTypeToString(ActionType type);
    static std::string processingModeToString(ProcessingMode mode);
};

} // namespace Mantids30::Servers::RESTfulWebServer
