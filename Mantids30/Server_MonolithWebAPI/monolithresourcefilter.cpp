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
        bool allPermissionsAreValid=true;

        // If the filter require a session and there is no session, it does not match...
        if (allPermissionsAreValid && filter.requireSession && !session)
            allPermissionsAreValid=false;

        // To check for required attributes, we have to have a session, then, if no session, not match:
        if (allPermissionsAreValid && !filter.requiredPermissions.empty() && !session)
            allPermissionsAreValid=false;

        // To check for rejected attributes, we have to have a session, then, if no session, not match:
        if (allPermissionsAreValid && !filter.rejectedPermissions.empty() && !session)
            allPermissionsAreValid=false;

        // If there is any missing permission:
        if (allPermissionsAreValid)
        {
            for (const auto &requiredPermission : filter.requiredPermissions)
            {
                if (!session->validateAppPermissionInClaim(requiredPermission))
                {
                    allPermissionsAreValid = false;
                    break;
                }
            }
        }

        // If there is any rejected permission:
        if (allPermissionsAreValid)
        {
            for (const auto &rejectedPermission : filter.rejectedPermissions)
            {
                if (session->validateAppPermissionInClaim(rejectedPermission))
                    allPermissionsAreValid = false;
            }
        }

        if (!allPermissionsAreValid)
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
        bool allPermissionsDone=true;

        for (const auto & permission : filter.requiredPermissions)
        {
            if (!hSession || ! identityManager) // Any Permission without the session is marked as false
                allPermissionsDone = false;
            else if (permission == "loggedin" && hSession->getAuthUser() == "")
                allPermissionsDone = false;
            else if (! identityManager->validateAccountApplicationPermission(hSession->getAuthUser(),{hSession->getApplicationName(), permission}))
                allPermissionsDone = false;
        }
        for (const auto & permission : filter.rejectedPermissions)
        {
            if (hSession &&  identityManager)
            {
                if (permission == "loggedin" && hSession->getAuthUser() != "")
                    allPermissionsDone = false;
                else if ( identityManager->validateAccountApplicationPermission(hSession->getAuthUser(),{hSession->getApplicationName(), permission}))
                    allPermissionsDone = false;
            }
        }

        if (!allPermissionsDone)
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
