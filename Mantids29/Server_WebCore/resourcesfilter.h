#pragma once

#include <string>
#include <list>
#include <set>
#include <map>
#include <Mantids29/Helpers/json.h>


#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1
#include <boost/regex.hpp>

namespace Mantids29 { namespace API { namespace Web {

struct UserData {

//    std::string ipAddress = "";
    std::string userName = "";
    std::string domainName = "";
  //  std::string tlsCommonName = "";

    std::set<std::string> attributes;
    std::map<std::string, json> claims;

    std::string halfSessionId = "";
    std::string sessionId = "";

    bool sessionActive = false;
    bool loggedIn = false;
};

class ResourcesFilter
{
public:
    ResourcesFilter();

    struct FilterEvaluationResult
    {
        bool accept = true;
        std::string redirectLocation = "";
    };

    enum FilterAction
    {
        RFILTER_ACCEPT=0,
        RFILTER_DENY=1,
        RFILTER_REDIRECT=2
    };

    struct Filter
    {
        void compileRegex()
        {
            for ( const auto &i : sRegexs )
            {
                this->regexPatterns.push_back( boost::regex(i.c_str(),boost::regex::extended ));
            }
        }

        std::list<boost::regex> regexPatterns;
        std::string redirectLocation = "";
        std::list<std::string> reqAttrib, rejAttrib, sRegexs;
        bool requireSession = false;
        bool requireLogin = false;
        bool disallowSession = false;
        bool disallowLogin = false;

        FilterAction action = RFILTER_ACCEPT;
    };

    bool loadFiltersFromFile(const std::string & filePath);
    void addFilter(const Filter & filter);


    /**
     * @brief Evaluates a given URI against a set of filters to determine the appropriate action.
     *
     * This function checks the URI against filters, each containing required attributes, rejected attributes,
     * regex patterns, and a specified action. If a filter matches the user's data and the URI,
     * the function performs the corresponding action (accept, redirect, or deny). If no filters match,
     * the function accepts the URI by default.
     *
     * @param uri The URI to be evaluated.
     * @param userData A pointer to the UserData object containing user-specific data.
     * @return A FilterEvaluationResult object containing the evaluation results, including whether to accept
     *         or deny the URI, and an optional redirect location.
     */
    FilterEvaluationResult evaluateURI(const std::string & uri, UserData * userData);

protected:

    std::list<Filter> filters;
};

}}}

