#include "monolithresourcefilter.h"
/*

using namespace boost;
using namespace Mantids30::API::Monolith;

// TODO: destroy this class or compare with the resource filter from webcore.

ResourcesFilter::ResourcesFilter()
{

}

ResourcesFilter::FilterEvaluationResult ResourcesFilter::evaluateURIWithSession(const std::string &uri, Sessions::Session *session)
{
    FilterEvaluationResult evaluationResult;

    int ruleIndex=0;
    for (const auto & filter : filters)
    {
        ruleIndex++;
        bool allScopesAreValid=true;

        // If the filter require a session and there is no session, it does not match...
        if (allScopesAreValid && filter.requireSession && !session)
            allScopesAreValid=false;

        // To check for required attributes, we have to have a session, then, if no session, not match:
        if (allScopesAreValid && !filter.requiredScopes.empty() && !session)
            allScopesAreValid=false;

        // To check for rejected attributes, we have to have a session, then, if no session, not match:
        if (allScopesAreValid && !filter.rejectedScopes.empty() && !session)
            allScopesAreValid=false;

        // If there is any missing scope:
        if (allScopesAreValid)
        {
            for (const auto &requiredScope : filter.requiredScopes)
            {
                if (!session->validateAppScopeInClaim(requiredScope))
                {
                    allScopesAreValid = false;
                    break;
                }
            }
        }

        // If there is any rejected scope:
        if (allScopesAreValid)
        {
            for (const auto &rejectedScope : filter.rejectedScopes)
            {
                if (session->validateAppScopeInClaim(rejectedScope))
                    allScopesAreValid = false;
            }
        }

        if (!allScopesAreValid)
        {
            // Rule match
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
    }

    evaluationResult.accept = true;
    return evaluationResult;
}
*/
/*
ResourcesFilter::sFilterEvaluation ResourcesFilter::evaluateAction(const std::string &uri, Mantids30::Sessions::Session * hSession, Mantids30::Sessions::IdentityManager *  identityManager)
{
    sFilterEvaluation evalRet;

    int ruleId=0;
    for (const auto & filter : filters)
    {
        ruleId++;
        bool allScopesDone=true;

        for (const auto & scope : filter.requiredScopes)
        {
            if (!hSession || ! identityManager) // Any Scope without the session is marked as false
                allScopesDone = false;
            else if (scope == "loggedin" && hSession->getAuthUser() == "")
                allScopesDone = false;
            else if (! identityManager->validateAccountApplicationScope(hSession->getAuthUser(),{hSession->getApplicationName(), scope}))
                allScopesDone = false;
        }
        for (const auto & scope : filter.rejectedScopes)
        {
            if (hSession &&  identityManager)
            {
                if (scope == "loggedin" && hSession->getAuthUser() != "")
                    allScopesDone = false;
                else if ( identityManager->validateAccountApplicationScope(hSession->getAuthUser(),{hSession->getApplicationName(), scope}))
                    allScopesDone = false;
            }
        }

        if (!allScopesDone)
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
