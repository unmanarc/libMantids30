#include "resourcesfilter.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>

using namespace Mantids30::API::Web;

namespace {

using PTree = boost::property_tree::ptree;
using Action = ResourcesFilter::Action;
using ActionType = ResourcesFilter::ActionType;
using ProcessingMode = ResourcesFilter::ProcessingMode;
using Filter = ResourcesFilter::Filter;

std::string normalizeIdentifier(std::string value)
{
    boost::algorithm::trim(value);
    boost::algorithm::to_upper(value);

    std::replace(value.begin(), value.end(), '-', '_');
    std::replace(value.begin(), value.end(), ' ', '_');

    return value;
}

ProcessingMode parseProcessingMode(const std::string &value)
{
    const std::string normalized = normalizeIdentifier(value);

    if (normalized == "HTMLIENGINE" || normalized == "HTMLI_ENGINE" || normalized == "HTMLI")
    {
        return ProcessingMode::HTMLIENGINE;
    }

    if (normalized == "MANTIDSLANG" || normalized == "MANTIDS_LANG")
    {
        return ProcessingMode::MANTIDSLANG;
    }

    return ProcessingMode::RAW;
}

uint16_t parseStatusCode(const PTree &node, uint16_t defaultStatusCode)
{
    unsigned int statusCode = node.get<unsigned int>("statusCode", node.get<unsigned int>("code", defaultStatusCode));

    if (statusCode > std::numeric_limits<uint16_t>::max())
    {
        throw std::runtime_error("HTTP status code is out of range");
    }

    return static_cast<uint16_t>(statusCode);
}

void loadStringList(const PTree &node, const std::string &childName, std::list<std::string> &destination)
{
    const auto child = node.get_child_optional(childName);

    if (!child)
    {
        return;
    }

    for (const auto &item : child.get())
    {
        destination.push_back(item.second.get_value<std::string>());
    }
}

void loadHeaders(const PTree &node, std::vector<std::pair<std::string, std::string>> &headers)
{
    for (const auto &header : node)
    {
        headers.emplace_back(header.first, header.second.get_value<std::string>());
    }
}

Action parseAction(const PTree &actionNode)
{
    std::string actionName = actionNode.get<std::string>("type", actionNode.get<std::string>("action", actionNode.get_value<std::string>("")));

    actionName = normalizeIdentifier(actionName);

    Action action;

    if (actionName == "REPLACE_HEADERS")
    {
        action.type = ActionType::REPLACE_HEADERS;

        auto headers = actionNode.get_child_optional("headers");

        if (!headers)
        {
            headers = actionNode.get_child_optional("httpExtraHeaders");
        }

        if (headers)
        {
            loadHeaders(headers.get(), action.httpHeaders);
        }

        return action;
    }

    if (actionName == "ADD_HEADERS")
    {
        action.type = ActionType::ADD_HEADERS;

        auto headers = actionNode.get_child_optional("headers");

        if (!headers)
        {
            headers = actionNode.get_child_optional("httpExtraHeaders");
        }

        if (headers)
        {
            loadHeaders(headers.get(), action.httpHeaders);
        }

        return action;
    }

    if (actionName == "PROCESS_AS" || actionName == "PROCESS")
    {
        action.type = ActionType::PROCESS_AS;
        action.processingMode = parseProcessingMode(actionNode.get<std::string>("mode", actionNode.get<std::string>("processingMode", "RAW")));

        return action;
    }

    if (actionName == "PROCESS_AS_HTMLI" || actionName == "PROCESS_AS_HTMLIENGINE" || actionName == "PROCESS_AS_HTMLI_ENGINE")
    {
        action.type = ActionType::PROCESS_AS;
        action.processingMode = ProcessingMode::HTMLIENGINE;
        return action;
    }

    if (actionName == "PROCESS_AS_MANTIDSLANG" || actionName == "PROCESS_AS_MANTIDS_LANG")
    {
        action.type = ActionType::PROCESS_AS;
        action.processingMode = ProcessingMode::MANTIDSLANG;
        return action;
    }

    if (actionName == "PROCESS_AS_RAW")
    {
        action.type = ActionType::PROCESS_AS;
        action.processingMode = ProcessingMode::RAW;
        return action;
    }

    if (actionName == "REDIRECT")
    {
        action.type = ActionType::REDIRECT;
        action.redirectLocation = actionNode.get<std::string>("location", actionNode.get<std::string>("redirectLocation", ""));

        action.statusCode = parseStatusCode(actionNode, 302);
        return action;
    }

    if (actionName == "DENY")
    {
        action.type = ActionType::DENY;
        action.statusCode = parseStatusCode(actionNode, 403);
        return action;
    }

    if (actionName == "ACCEPT" || actionName == "ALLOW")
    {
        action.type = ActionType::ACCEPT;
        action.statusCode = parseStatusCode(actionNode, 200);
        return action;
    }

    throw std::runtime_error("Unknown resource-filter action: " + actionName);
}

bool containsAll(const std::set<std::string> &values, const std::list<std::string> &requiredValues)
{
    for (const std::string &requiredValue : requiredValues)
    {
        if (values.find(requiredValue) == values.end())
        {
            return false;
        }
    }

    return true;
}

bool containsNone(const std::set<std::string> &values, const std::list<std::string> &rejectedValues)
{
    for (const std::string &rejectedValue : rejectedValues)
    {
        if (values.find(rejectedValue) != values.end())
        {
            return false;
        }
    }

    return true;
}

bool filterRequirementsMatch(const Filter &filter, const std::set<std::string> &scopes, const std::set<std::string> &roles, bool isSessionActive, bool isAdmin)
{
    if (!containsAll(scopes, filter.requiredScopes) && !isAdmin)
    {
        return false;
    }

    if (!containsNone(scopes, filter.rejectedScopes))
    {
        return false;
    }

    if (!containsAll(roles, filter.requiredRoles) && !isAdmin)
    {
        return false;
    }

    if (!containsNone(roles, filter.rejectedRoles))
    {
        return false;
    }

    if (filter.requireSession && !isSessionActive)
    {
        return false;
    }

    if (filter.disallowSession && isSessionActive)
    {
        return false;
    }

    return true;
}

bool isTerminalAction(ActionType type)
{
    return type == ActionType::REDIRECT ||
           type == ActionType::ACCEPT ||
           type == ActionType::DENY;
}

bool filterURIMatches(const Filter &filter, const std::string &uri)
{
    boost::cmatch match;

    for (const boost::regex &pattern : filter.regexPatterns)
    {
        if (boost::regex_match(uri.c_str(), match, pattern))
        {
            return true;
        }
    }

    return false;
}

} // namespace

void ResourcesFilter::Filter::compileRegex()
{
    regexPatterns.clear();

    for (const std::string &regex : uriRegexs)
    {
        regexPatterns.emplace_back(regex.c_str(), boost::regex::extended);
    }
}

bool ResourcesFilter::loadFiltersFromFile(const std::string &filePath)
{
    PTree root;
    boost::property_tree::read_json(filePath, root);

    std::list<Filter> loadedFilters;

    for (const auto &filterNode : root)
    {
        Filter filter;

        loadStringList(filterNode.second, "uriRegexs", filter.uriRegexs);
        loadStringList(filterNode.second, "requiredScopes", filter.requiredScopes);
        loadStringList(filterNode.second, "disallowedScopes", filter.rejectedScopes);
        loadStringList(filterNode.second, "requiredRoles", filter.requiredRoles);
        loadStringList(filterNode.second, "disallowedRoles", filter.rejectedRoles);
        filter.requireSession = filterNode.second.get<bool>("requireSession", false);
        filter.disallowSession = filterNode.second.get<bool>("disallowSession", false);
        const auto actions = filterNode.second.get_child_optional("actions");

        if (actions)
        {
            for (const auto &actionNode : actions.get())
            {
                filter.actions.push_back(parseAction(actionNode.second));
            }
        }

        filter.compileRegex();
        loadedFilters.push_back(std::move(filter));
    }

    m_filters.splice(m_filters.end(), loadedFilters);

    return true;
}

void ResourcesFilter::addFilter(const Filter &filter)
{
    Filter compiledFilter = filter;
    compiledFilter.compileRegex();

    m_filters.push_back(std::move(compiledFilter));
}

void ResourcesFilter::clearFilters()
{
    m_filters.clear();
}

ResourcesFilter::FilterEvaluationResult ResourcesFilter::evaluateURI(const std::string &uri, const std::set<std::string> &scopes, const std::set<std::string> &roles, bool isSessionActive, bool isAdmin) const
{
    FilterEvaluationResult result;

    for (const Filter &filter : m_filters)
    {
        if (!filterRequirementsMatch(filter, scopes, roles, isSessionActive,isAdmin))
        {
            continue;
        }

        if (!filterURIMatches(filter, uri))
        {
            continue;
        }

        result.matched = true;

        for (const auto &action : filter.actions)
        {
            result.actions.push_back(action);

            if (isTerminalAction(action.type))
            {
                break;
            }
        }

        if (isTerminalAction(result.actions.back().type))
        {
            break;
        }
    }

    if (!result.matched)
    {
        Action defaultAction;
        defaultAction.type = ActionType::ACCEPT;
        defaultAction.statusCode = 200;

        result.actions.push_back(std::move(defaultAction));
    }

    return result;
}

namespace {

std::string actionTypeToString(ActionType type)
{
    switch (type)
    {
    case ActionType::ADD_HEADERS: return "ADD_HEADERS";
    case ActionType::REPLACE_HEADERS: return "REPLACE_HEADERS";
    case ActionType::PROCESS_AS: return "PROCESS_AS";
    case ActionType::REDIRECT: return "REDIRECT";
    case ActionType::ACCEPT: return "ACCEPT";
    case ActionType::DENY: return "DENY";
    }
    return "UNKNOWN";
}

std::string processingModeToString(ProcessingMode mode)
{
    switch (mode)
    {
    case ProcessingMode::RAW: return "RAW";
    case ProcessingMode::HTMLIENGINE: return "HTMLIENGINE";
    case ProcessingMode::MANTIDSLANG: return "MANTIDSLANG";
    }
    return "UNKNOWN";
}

} // namespace

Json::Value ResourcesFilter::FilterEvaluationResult::toJSON() const
{
    Json::Value root;

    root["matched"] = matched;

    Json::Value actionsArray(Json::arrayValue);
    for (const auto &action : actions)
    {
        Json::Value actionObj;
        actionObj["type"] = actionTypeToString(action.type);
        actionObj["statusCode"] = static_cast<Json::Int>(action.statusCode);

        if (!action.redirectLocation.empty())
        {
            actionObj["redirectLocation"] = action.redirectLocation;
        }

        actionObj["processingMode"] = processingModeToString(action.processingMode);

        if (!action.httpHeaders.empty())
        {
            Json::Value headersArray(Json::arrayValue);
            for (const auto &header : action.httpHeaders)
            {
                Json::Value headerObj;
                headerObj["name"] = header.first;
                headerObj["value"] = header.second;
                headersArray.append(headerObj);
            }
            actionObj["httpHeaders"] = headersArray;
        }

        actionsArray.append(actionObj);
    }
    root["actions"] = actionsArray;

    return root;
}
