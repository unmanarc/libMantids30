#pragma once

#include <boost/regex.hpp>

#include <cstdint>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Mantids30::API::Web {

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

    [[nodiscard]] FilterEvaluationResult evaluateURI(const std::string &uri, const std::set<std::string> &scopes, const std::set<std::string> &roles, bool isSessionActive) const;

protected:
    std::list<Filter> m_filters;
};

} // namespace Mantids30::API::Web
