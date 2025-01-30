#include "methodshandler.h"
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Threads/lock_shared.h>
#include <memory>

using namespace Mantids30::API::Monolith;
using namespace Mantids30;

MethodsHandler::MethodsHandler(/*const std::string &appName*/)
{
    //this->m_applicationName = appName;
}

bool MethodsHandler::addMethod(const MethodDefinition &methodDefinition)
{
    Threads::Sync::Lock_RW lock(m_methodsMutex);
    if (m_methods.find(methodDefinition.methodName) == m_methods.end() )
    {
        // Put the method.
        m_methods[methodDefinition.methodName] = methodDefinition.method;

        // Configure methodsPermissions with this info.
        m_methodsPermissions.addMethodRequiredPermissions(methodDefinition.methodName,methodDefinition.reqPermissions);
        m_methodsPermissions.addMethodRequiredRoles(methodDefinition.methodName,methodDefinition.reqRoles);

        m_methodRequireActiveSession[methodDefinition.methodName] = methodDefinition.isActiveSessionRequired;
        m_methodUpdateSessionLastActivityOnUsage[methodDefinition.methodName] = methodDefinition.doUsageUpdateLastSessionActivity;

        return true;
    }
    return false;
}

int MethodsHandler::invoke(std::shared_ptr<Mantids30::Sessions::Session> session, const std::string & methodName, const json & payload,  json * payloadOut)
{
    Threads::Sync::Lock_RD lock(m_methodsMutex);

    if (m_methods.find(methodName) == m_methods.end())
        return METHOD_RET_CODE_METHODNOTFOUND;
    else
    {
        *payloadOut = m_methods[methodName].method(m_methods[methodName].context, session, payload);

        if ( m_methodUpdateSessionLastActivityOnUsage[methodName] )
        {
            session->updateLastActivity();
        }

        return METHOD_RET_CODE_SUCCESS;
    }
}

MethodsHandler::eMethodValidationCodes MethodsHandler::validateMethodRequirements(std::shared_ptr<Mantids30::Sessions::Session> session, const std::string & methodName, json * reasons)
{
    std::set<std::string> permissionsLeft, rolesLeft;
    Threads::Sync::Lock_RD lock(m_methodsMutex);

    // Check if the method exist at all:
    if (m_methods.find(methodName) == m_methods.end())
        return VALIDATION_METHODNOTFOUND;

    // If requires full authentication, check that the session report that is fully authenticated (all required ID's) and it's also a persistent session.
    if (m_methodRequireActiveSession[methodName])
    {
        if (!session)
            return VALIDATION_NOTAUTHORIZED;
    }

    // Validate that the method haves the required permissions/auth slot ids:
    if (m_methodsPermissions.validateMethod(session,methodName,rolesLeft,permissionsLeft))
    {
        return VALIDATION_OK;
    }
    else
    {
        // The method is not authorized for this authentication level.. Report what is failing.
        (*reasons)["rolesLeft"] = Helpers::setToJSON(rolesLeft);
        (*reasons)["permissionsLeft"] = Helpers::setToJSON(permissionsLeft);
        return VALIDATION_NOTAUTHORIZED;
    }

}

MethodsRequirements_Map *MethodsHandler::methodsRequirements()
{
    return &m_methodsPermissions;
}

bool MethodsHandler::doesMethodRequireActiveSession(const std::string &methodName)
{
    Threads::Sync::Lock_RD lock(m_methodsMutex);
    return m_methodRequireActiveSession[methodName];
}
/*
std::set<std::string> MethodsHandler::getApplicationPermissions(const std::set<std::string> &reqPermissions)
{
    std::set<std::string> r;
    for (const auto &i : reqPermissions)
    {
        r.insert({m_applicationName,i});
    }
    return r;
}*/
/*
std::string MethodsHandler::getApplicationName() const
{
    return m_applicationName;
}*/
