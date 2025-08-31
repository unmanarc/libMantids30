#include "methodsrequirements_map.h"
#include <memory>
#include <string>

using namespace Mantids30::API::Monolith;

void MethodsRequirements_Map::addMethodRequiredScopes(const std::string &methodName, const std::set<std::string> &applicationScopes)
{
    for (const std::string & scope: applicationScopes)
        m_methodRequiredScopes.insert(std::make_pair(methodName, scope));
}

std::set<std::string> MethodsRequirements_Map::getMethodRequiredScopes(const std::string &methodName)
{
    std::set<std::string> r;
    auto it = m_methodRequiredScopes.equal_range(methodName);
    for (auto itr = it.first; itr != it.second; ++itr)
        r.insert(itr->second);
    return r;
}

void MethodsRequirements_Map::addMethodRequiredRoles(const std::string &methodName, const std::set<std::string> &applicationRoles)
{
    for (const std::string & role : applicationRoles)
        m_methodRequiredRoles.insert(std::make_pair(methodName, role));
}

std::set<std::string> MethodsRequirements_Map::getMethodRequiredRoles(const std::string &methodName)
{
    std::set<std::string> r;
    auto it = m_methodRequiredRoles.equal_range(methodName);
    for (auto itr = it.first; itr != it.second; ++itr)
        r.insert(itr->second);
    return r;
}

bool MethodsRequirements_Map::validateMethod(std::shared_ptr<Sessions::Session> session, const std::string &methodName, std::set<std::string> &rolesLeft, std::set<std::string> &scopesLeft)
{
    if (!session)
        return false;

    bool isAdmin = session->getJWTAuthenticatedInfo().isAdmin();
    std::set<std::string> authedRoles = session->getJWTAuthenticatedInfo().getAllRoles();

    if (isAdmin)
    {
        rolesLeft.clear();
        scopesLeft.clear();
        return true;
    }

    std::set<std::string> requiredRoles  = getMethodRequiredRoles(methodName);
    std::set<std::string> requiredScopes = getMethodRequiredScopes(methodName);

    // start with all required scopes/slotIds...
    rolesLeft = rolesLeft;
    scopesLeft = requiredScopes;

    for ( const std::string & scope : requiredScopes )
    {
        if (session && session->getJWTAuthenticatedInfo().hasScope(scope))
            scopesLeft.erase(scope);
    }

    for ( const std::string & role : requiredRoles )
    {
        if (session && session->getJWTAuthenticatedInfo().hasRole(role))
            scopesLeft.erase(role);
    }

    return rolesLeft.empty() && scopesLeft.empty();
}

