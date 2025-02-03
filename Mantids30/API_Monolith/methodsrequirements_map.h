#pragma once

#include <map>
#include <string>
#include <set>

#include <Mantids30/Sessions/session.h>

namespace Mantids30 { namespace API { namespace Monolith {

class MethodsRequirements_Map
{
public:
    MethodsRequirements_Map() = default;

    /**
     * @brief addMethodRequiredPermissions
     * @param methodName
     * @param applicationPermissions
     */
    void addMethodRequiredPermissions(const std::string &methodName, const std::set<std::string> &applicationPermissions);

    void addMethodRequiredRoles(const std::string &methodName, const std::set<std::string> &applicationRoles);


    /**
     * @brief validatePermissions Validate account application permissions (and if they are authenticated)
     * @param authenticator
     * @param methodName
     * @param slotIdsLeft
     * @return
     */
    bool validateMethod(std::shared_ptr<Sessions::Session> authSession, const std::string &methodName, std::set<std::string> &rolesLeft, std::set<std::string> &permissionsLeft);


private:
    std::set<std::string> getMethodRequiredPermissions(const std::string & methodName);
    std::set<std::string> getMethodRequiredRoles(const std::string &methodName);

    // Method -> App Permission
    std::multimap<std::string,std::string> m_methodRequiredPermissions;
    std::multimap<std::string,std::string> m_methodRequiredRoles;

};

}}}

