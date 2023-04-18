#include "monolithresourcefilter.h"


using namespace boost;
using namespace Mantids29::API::Monolith;

ResourcesFilter::ResourcesFilter()
{

}

ResourcesFilter::FilterEvaluationResult ResourcesFilter::evaluateURIWithSession(const std::string &uri, Authentication::Session *userSession, Authentication::Manager *authorizer)
{
    FilterEvaluationResult evaluationResult;

    int ruleIndex=0;
    for (const auto & filter : filters)
    {
        ruleIndex++;
        bool allAttributesValid=true;

        for (const auto & requiredAttribute : filter.reqAttrib)
        {
            if (!userSession || !authorizer) // Any attribute without the session is marked as false
                allAttributesValid = false;
            else if (requiredAttribute == "loggedin" && userSession->getAuthUser() == "")
                allAttributesValid = false;
            else if (!authorizer->validateAccountAttribute(userSession->getAuthUser(),{userSession->getApplicationName(), requiredAttribute}))
                allAttributesValid = false;
        }
        for (const auto & rejectedAttribute : filter.rejAttrib)
        {
            if (userSession && authorizer)
            {
                if (rejectedAttribute == "loggedin" && userSession->getAuthUser() != "")
                    allAttributesValid = false;
                else if (authorizer->validateAccountAttribute(userSession->getAuthUser(),{userSession->getApplicationName(), rejectedAttribute}))
                    allAttributesValid = false;
            }
        }

        if (!allAttributesValid)
        {
            continue; // Rule does not match
        }

        boost::cmatch matchedRegex;
        for (const auto & regexPattern : filter.regexPatterns)
        {
            if (boost::regex_match(uri.c_str(), matchedRegex, regexPattern))
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

    evaluationResult.accept = true;
    return evaluationResult;
}

/*
ResourcesFilter::sFilterEvaluation ResourcesFilter::evaluateAction(const std::string &uri, Mantids29::Authentication::Session * hSession, Mantids29::Authentication::Manager * authorizer)
{
    sFilterEvaluation evalRet;

    int ruleId=0;
    for (const auto & filter : filters)
    {
        ruleId++;
        bool allAttribsDone=true;

        for (const auto & attrib : filter.reqAttrib)
        {
            if (!hSession || !authorizer) // Any attribute without the session is marked as false
                allAttribsDone = false;
            else if (attrib == "loggedin" && hSession->getAuthUser() == "")
                allAttribsDone = false;
            else if (!authorizer->validateAccountAttribute(hSession->getAuthUser(),{hSession->getApplicationName(), attrib}))
                allAttribsDone = false;
        }
        for (const auto & attrib : filter.rejAttrib)
        {
            if (hSession && authorizer)
            {
                if (attrib == "loggedin" && hSession->getAuthUser() != "")
                    allAttribsDone = false;
                else if (authorizer->validateAccountAttribute(hSession->getAuthUser(),{hSession->getApplicationName(), attrib}))
                    allAttribsDone = false;
            }
        }

        if (!allAttribsDone)
        {
            continue; // Rule does not match
        }

        boost::cmatch what;
        for (const auto & i : filter.regexs)
        {
            if (boost::regex_match(uri.c_str(), what, i))
            {
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
*/
