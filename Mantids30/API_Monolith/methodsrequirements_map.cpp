#include "methodsrequirements_map.h"
#include <memory>
#include <string>

using namespace Mantids30::API::Monolith;

MethodsRequirements_Map::MethodsRequirements_Map()
{
}

void MethodsRequirements_Map::addMethodRequiredPermissions(const std::string &methodName, const std::set<std::string> &applicationPermissions)
{
    for (const std::string & permission : applicationPermissions)
        m_methodRequiredPermissions.insert(std::make_pair(methodName, permission));
}

std::set<std::string> MethodsRequirements_Map::getMethodRequiredPermissions(const std::string &methodName)
{
    std::set<std::string> r;
    auto it = m_methodRequiredPermissions.equal_range(methodName);
    for (auto itr = it.first; itr != it.second; ++itr)
        r.insert(itr->second);
    return r;
}

void MethodsRequirements_Map::addMethodRequiredRoles(const std::string &methodName, const std::set<std::string> &applicationRoles)
{
    for (const std::string & permission : applicationRoles)
        m_methodRequiredRoles.insert(std::make_pair(methodName, permission));
}

std::set<std::string> MethodsRequirements_Map::getMethodRequiredRoles(const std::string &methodName)
{
    std::set<std::string> r;
    auto it = m_methodRequiredRoles.equal_range(methodName);
    for (auto itr = it.first; itr != it.second; ++itr)
        r.insert(itr->second);
    return r;
}

bool MethodsRequirements_Map::validateMethod(std::shared_ptr<Sessions::Session> session, const std::string &methodName, std::set<std::string> &rolesLeft, std::set<std::string> &permissionsLeft)
{
    if (!session)
        return false;

    bool isAdmin = session->getJWTAuthenticatedInfo().isAdmin();
    std::set<std::string> authedRoles = session->getJWTAuthenticatedInfo().getAllRoles();

    if (isAdmin)
    {
        rolesLeft.clear();
        permissionsLeft.clear();
        return true;
    }

    std::set<std::string> requiredRoles  = getMethodRequiredRoles(methodName);
    std::set<std::string> requiredPermissions = getMethodRequiredPermissions(methodName);

    // start with all required permissions/slotIds...
    rolesLeft = rolesLeft;
    permissionsLeft = requiredPermissions;

    for ( const std::string & permission : requiredPermissions )
    {
        if (session && session->getJWTAuthenticatedInfo().hasPermission(permission))
            permissionsLeft.erase(permission);
    }

    for ( const std::string & role : requiredRoles )
    {
        if (session && session->getJWTAuthenticatedInfo().hasRole(role))
            permissionsLeft.erase(role);
    }

    return rolesLeft.empty() && permissionsLeft.empty();
}

