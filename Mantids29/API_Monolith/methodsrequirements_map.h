#pragma once

#include <map>
#include <string>
#include <set>

#include <Mantids29/Auth/session.h>

namespace Mantids29 { namespace Auth {

class MethodsRequirements_Map
{
public:
    MethodsRequirements_Map();

    /**
     * @brief addMethodRequiredPermissions
     * @param methodName
     * @param applicationPermissions
     */
    void addMethodRequiredPermissions(const std::string &methodName, const std::set<std::string> &applicationPermissions);

    void addMethodRequiredActivities(const std::string &methodName, const std::set<std::string> &applicationActivities);


    /**
     * @brief validatePermissions Validate account application permissions (and if they are authenticated)
     * @param authenticator
     * @param methodName
     * @param slotIdsLeft
     * @return
     */
    bool validateMethod(Session *authSession, const std::string &methodName, std::set<std::string> &activitiesLeft, std::set<std::string> &permissionsLeft);


private:
    std::set<std::string> getMethodRequiredPermissions(const std::string & methodName);
    std::set<std::string> getMethodRequiredActivities(const std::string &methodName);

    // Method -> App Permission
    std::multimap<std::string,std::string> m_methodRequiredPermissions;
    std::multimap<std::string,std::string> m_methodRequiredActivities;

};

}}

