#include "resourcesfilter.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string/case_conv.hpp>



using namespace boost;
using namespace Mantids30::API::Web;

ResourcesFilter::ResourcesFilter()
{
}

bool ResourcesFilter::loadFiltersFromFile(const std::string &filePath)
{
    // Create a root ptree
    property_tree::ptree root;

    // TODO:
    //    try {
    // Load the info file in this ptree
    property_tree::read_json(filePath, root);
    /*  }
    catch (property_tree::info_parser_error x)
    {
        return false;
    }*/

    /* try
    {*/
    for (const auto & i : root)
    {
        Filter filter;

        auto pRegexs = i.second.get_child_optional("uriRegex");
        if (pRegexs)
        {
            for (const auto & i : pRegexs.get())
            {
                filter.sRegexs.push_back(i.second.get_value<std::string>());
            }
        }
        filter.compileRegex();


        auto pRequiredPermissions = i.second.get_child_optional("requiredPermissions");
        if (pRequiredPermissions)
        {
            for (const auto & i : pRequiredPermissions.get())
            {
                filter.requiredPermissions.push_back(i.second.get_value<std::string>());
            }
        }

        auto pDisallowedPermissions =  i.second.get_child_optional("disallowedPermissions");
        if (pDisallowedPermissions)
        {
            for (const auto & i : pDisallowedPermissions.get())
            {
                filter.rejectedPermissions.push_back(i.second.get_value<std::string>());
            }
        }

        auto pRequiredRoles = i.second.get_child_optional("requiredRoles");
        if (pRequiredRoles)
        {
            for (const auto & i : pRequiredRoles.get())
            {
                filter.requiredRoles.push_back(i.second.get_value<std::string>());
            }
        }

        auto pDisallowedRoles =  i.second.get_child_optional("disallowedRoles");
        if (pDisallowedRoles)
        {
            for (const auto & i : pDisallowedRoles.get())
            {
                filter.rejectedRoles.push_back(i.second.get_value<std::string>());
            }
        }

        // Action is mandatory:
        std::string sAction = boost::to_upper_copy(i.second.get<std::string>("action"));
        if ( sAction == "REDIRECT" ) filter.action = RFILTER_REDIRECT;
        if ( sAction == "DENY" ) filter.action = RFILTER_DENY;
        if ( sAction == "ACCEPT" ) filter.action = RFILTER_ACCEPT;

        filter.redirectLocation = i.second.get_optional<std::string>("redirectLocation")?i.second.get<std::string>("redirectLocation"):"";

        //filter.requireLogin = i.second.get<bool>("requireLogin",false);
        filter.requireSession = i.second.get<bool>("requireSession",false);

        //filter.disallowLogin = i.second.get<bool>("disallowLogin",false);
        filter.disallowSession = i.second.get<bool>("disallowSession",false);

        addFilter( filter );
    }
    /* }
    catch(property_tree::ptree_bad_path)
    {
        return false;
    }
    Explode and explain...
*/

    return true;
}

void ResourcesFilter::addFilter(const Filter &filter)
{
    filters.push_back(filter);
}

ResourcesFilter::FilterEvaluationResult ResourcesFilter::evaluateURI(const std::string &uri, const std::set<std::string> & permissions,const std::set<std::string> & roles, bool isSessionActive)
{
    FilterEvaluationResult evaluationResult;

    for (const Filter & filter : filters)
    {
        // Evaluate the filter:

        // Set the match to true...
        bool filterMatchesRequirements=true;

        // Check required permissions
        for (const auto & requiredPermission : filter.requiredPermissions)
        {
            if (!filterMatchesRequirements)
                break;

            if ( permissions.find(requiredPermission) == permissions.end() )
                filterMatchesRequirements = false;
        }

        // Check rejected permissions
        for (const auto & rejectedPermission : filter.rejectedPermissions)
        {
            if (!filterMatchesRequirements)
                break;

            if ( permissions.find(rejectedPermission) != permissions.end() )
                filterMatchesRequirements = false;
        }


        // Check required roles
        for (const auto & requiredRole : filter.requiredRoles)
        {
            if (!filterMatchesRequirements)
                break;

            if ( roles.find(requiredRole) == roles.end() )
                filterMatchesRequirements = false;
        }

        // Check rejected roles
        for (const auto & rejectedRole : filter.rejectedRoles)
        {
            if (!filterMatchesRequirements)
                break;

            if ( roles.find(rejectedRole) != roles.end() )
                filterMatchesRequirements = false;
        }


        // Check if the user needs to have an active session
        if (filter.requireSession && !isSessionActive)
            filterMatchesRequirements = false;

        // Check if the user needs not to have an active session
        if (filter.disallowSession && isSessionActive)
            filterMatchesRequirements = false;

        // Check if the user needs not to be logged in
/*        if (filter.disallowLogin && userData->loggedIn)
            filterMatchesRequirements = false;*/

        // Check if the user needs to be logged in
        /*        if (filter.requireLogin && !userData->loggedIn)
            filterMatchesRequirements = false;*/

        // If the filter doesn't match the requirements, continue with the next filter
        if (!filterMatchesRequirements)
        {
            continue; // Rule does not match
        }

        // Check if the URI matches any of the filter's regex patterns
        boost::cmatch what;
        for (const auto & uriRegexPattern : filter.regexPatterns)
        {
            if (boost::regex_match(uri.c_str(), what, uriRegexPattern))
            {
                switch (  filter.action  )
                {
                case RFILTER_ACCEPT:
                    evaluationResult.accept = true;
                    break;
                case RFILTER_REDIRECT:
                    evaluationResult.accept = true;
                    evaluationResult.redirectLocation = filter.redirectLocation;
                    break;
                case RFILTER_DENY:
                default:
                    evaluationResult.accept = false;
                    break;
                }
                return evaluationResult;
            }
        }
    }

    // If no filters match, accept the URI by default
    evaluationResult.accept = true;
    return evaluationResult;
}
