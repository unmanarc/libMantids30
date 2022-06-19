#include "resourcesfilter.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string/case_conv.hpp>

using namespace boost;
using namespace Mantids::RPC::Web;

ResourcesFilter::ResourcesFilter()
{
}

bool ResourcesFilter::loadFile(const std::string &filePath)
{
    // Create a root ptree
    property_tree::ptree root;

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
        std::list<std::string> regexs;
        auto pRegexs = i.second.get_child_optional("uriRegex");
        if (pRegexs)
        {
            for (const auto & i : pRegexs.get())
            {
                regexs.push_back(i.second.get_value<std::string>());
            }
        }

        std::list<std::string> requiredAppAtrribs;
        auto pRequiredAppAttribs = i.second.get_child_optional("requiredAppAtrribs");
        if (pRequiredAppAttribs)
        {
            for (const auto & i : pRequiredAppAttribs.get())
            {
                requiredAppAtrribs.push_back(i.second.get_value<std::string>());
            }
        }

        std::list<std::string> rejectedAppAtrribs;
        auto pRejectedAppAttribs =  i.second.get_child_optional("rejectedAppAtrribs");
        if (pRejectedAppAttribs)
        {
            for (const auto & i : pRejectedAppAttribs.get())
            {
                rejectedAppAtrribs.push_back(i.second.get_value<std::string>());
            }
        }

        eFilterActions fAction;

        // Action is mandatory:
        std::string sAction = boost::to_upper_copy(i.second.get<std::string>("action"));
        if ( sAction == "REDIRECT" ) fAction = RFILTER_REDIRECT;
        if ( sAction == "DENY" ) fAction = RFILTER_DENY;
        if ( sAction == "ACCEPT" ) fAction = RFILTER_ACCEPT;


        std::string sRedirectLocation = i.second.get_optional<std::string>("redirectLocation")?i.second.get<std::string>("redirectLocation"):"";

        addFilter(sFilter( regexs,
                           sRedirectLocation,
                           requiredAppAtrribs,
                           rejectedAppAtrribs,
                           fAction
                           ));
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

void Mantids::RPC::Web::ResourcesFilter::addFilter(const Mantids::RPC::Web::sFilter &filter)
{
    filters.push_back(filter);
}

sFilterEvaluation ResourcesFilter::evaluateAction(const std::string &uri, Mantids::Authentication::Session * hSession, Mantids::Authentication::Manager * authorizer)
{
    sFilterEvaluation evalRet;

    int ruleId=0;
    for (const auto & filter : filters)
    {
        ruleId++;
        bool allAttribsDone=true;

        /*printf("Evaluating rule %d for %s\n%s",  ruleId,uri.c_str(), filter.toString().c_str());
        fflush(stdout);*/

        for (const auto & attrib : filter.reqAttrib)
        {
            if (!hSession || !authorizer) // Any attribute without the session is marked as false
                allAttribsDone = false;
            else if (attrib == "loggedin" && hSession->getAuthUser() == "")
                allAttribsDone = false;
            else if (!authorizer->accountValidateAttribute(hSession->getAuthUser(),{hSession->getAppName(), attrib}))
                allAttribsDone = false;
        }
        for (const auto & attrib : filter.rejAttrib)
        {
            if (hSession && authorizer)
            {
                if (attrib == "loggedin" && hSession->getAuthUser() != "")
                    allAttribsDone = false;
                else if (authorizer->accountValidateAttribute(hSession->getAuthUser(),{hSession->getAppName(), attrib}))
                    allAttribsDone = false;
            }
        }

        if (!allAttribsDone)
        {
          /*  printf("\nRule %d does not match attributes.\n", ruleId);
            fflush(stdout);
*/
            continue; // Rule does not match
        }

       /* printf("\nRule %d match attributes, checking regex.\n", ruleId);
        fflush(stdout);
*/
        boost::cmatch what;
        for (const auto & i : filter.regexs)
        {
            if (boost::regex_match(uri.c_str(), what, i))
            {
              /*  printf("Rule %d match regex, returning action.\n", ruleId);
                fflush(stdout);
*/
                switch (  filter.action  )
                {
                case RFILTER_ACCEPT:
                    evalRet.accept = true;
                    break;
                case RFILTER_REDIRECT:
                    evalRet.accept = true;
                    evalRet.redirectLocation = filter.redirectLocation;
                    break;
                case RFILTER_DENY:
                default:
                    evalRet.accept = false;
                    break;
                }
                return evalRet;
            }
        }
    }

    evalRet.accept = true;
    return evalRet;
}
