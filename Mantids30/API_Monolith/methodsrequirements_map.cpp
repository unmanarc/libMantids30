#include "methodsrequirements_map.h"
#include <string>

using namespace Mantids30::Auth;

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

void MethodsRequirements_Map::addMethodRequiredActivities(const std::string &methodName, const std::set<std::string> &applicationActivities)
{
    for (const std::string & permission : applicationActivities)
        m_methodRequiredActivities.insert(std::make_pair(methodName, permission));
}

std::set<std::string> MethodsRequirements_Map::getMethodRequiredActivities(const std::string &methodName)
{
    std::set<std::string> r;
    auto it = m_methodRequiredActivities.equal_range(methodName);
    for (auto itr = it.first; itr != it.second; ++itr)
        r.insert(itr->second);
    return r;
}

bool MethodsRequirements_Map::validateMethod(Session *session, const std::string &methodName, std::set<std::string> &activitiesLeft, std::set<std::string> &permissionsLeft)
{
    bool isAdmin = session->isAdmin();
    std::set<std::string> authedActivities = Helpers::jsonToStringSet( session->getClaim("activities") );

    if (isAdmin)
    {
        activitiesLeft.clear();
        permissionsLeft.clear();
        return true;
    }

    std::set<std::string> requiredActivities  = getMethodRequiredActivities(methodName);
    std::set<std::string> requiredPermissions = getMethodRequiredPermissions(methodName);

    // start with all required permissions/slotIds...
    activitiesLeft = activitiesLeft;
    permissionsLeft = requiredPermissions;

    for ( const std::string & permission : requiredPermissions )
    {
        if (session && session->validateAppPermissionInClaim(permission))
            permissionsLeft.erase(permission);
    }

    return activitiesLeft.empty() && permissionsLeft.empty();
}

