#include "resourcesfilter.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string/case_conv.hpp>



using namespace boost;
using namespace Mantids29::API::Web;

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


        auto pRequiredAppAttribs = i.second.get_child_optional("requiredAppAtrribs");
        if (pRequiredAppAttribs)
        {
            for (const auto & i : pRequiredAppAttribs.get())
            {
                filter.reqAttrib.push_back(i.second.get_value<std::string>());
            }
        }

        auto pDisallowedAppAttribs =  i.second.get_child_optional("disallowedAppAtrribs");
        if (pDisallowedAppAttribs)
        {
            for (const auto & i : pDisallowedAppAttribs.get())
            {
                filter.rejAttrib.push_back(i.second.get_value<std::string>());
            }
        }

        // Action is mandatory:
        std::string sAction = boost::to_upper_copy(i.second.get<std::string>("action"));
        if ( sAction == "REDIRECT" ) filter.action = RFILTER_REDIRECT;
        if ( sAction == "DENY" ) filter.action = RFILTER_DENY;
        if ( sAction == "ACCEPT" ) filter.action = RFILTER_ACCEPT;


        filter.redirectLocation = i.second.get_optional<std::string>("redirectLocation")?i.second.get<std::string>("redirectLocation"):"";

        filter.requireLogin = i.second.get<bool>("requireLogin",false);
        filter.requireSession = i.second.get<bool>("requireSession",false);

        filter.disallowLogin = i.second.get<bool>("disallowLogin",false);
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

ResourcesFilter::FilterEvaluationResult ResourcesFilter::evaluateURI(const std::string &uri, UserData *userData)
{
    FilterEvaluationResult evaluationResult;

    for (const Filter & filter : filters)
    {
        // Evaluate the filter:

        // Set the match to true...
        bool filterMatchesRequirements=true;

        // Check required attributes
        for (const auto & requiredAttribute : filter.reqAttrib)
        {
            if (!filterMatchesRequirements)
                break;

            if ( userData->attributes.find(requiredAttribute) == userData->attributes.end() )
                filterMatchesRequirements = false;
        }

        // Check rejected attributes
        for (const auto & rejectedAttribute : filter.rejAttrib)
        {
            if (!filterMatchesRequirements)
                break;

            if ( userData->attributes.find(rejectedAttribute) != userData->attributes.end() )
                filterMatchesRequirements = false;
        }

        // Check if the user needs to be logged in
        if (filter.requireLogin && !userData->loggedIn)
            filterMatchesRequirements = false;

        // Check if the user needs to have an active session
        if (filter.requireSession && !userData->sessionActive)
            filterMatchesRequirements = false;

        // Check if the user needs not to be logged in
        if (filter.disallowLogin && userData->loggedIn)
            filterMatchesRequirements = false;

        // Check if the user needs not to have an active session
        if (filter.disallowSession && userData->sessionActive)
            filterMatchesRequirements = false;

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
