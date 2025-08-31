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
     * @brief addMethodRequiredScopes
     * @param methodName
     * @param applicationScopes
     */
    void addMethodRequiredScopes(const std::string &methodName, const std::set<std::string> &applicationScopes);

    void addMethodRequiredRoles(const std::string &methodName, const std::set<std::string> &applicationRoles);


    /**
     * @brief validateScopes Validate account application scopes (and if they are authenticated)
     * @param authenticator
     * @param methodName
     * @param slotIdsLeft
     * @return
     */
    bool validateMethod(std::shared_ptr<Sessions::Session> authSession, const std::string &methodName, std::set<std::string> &rolesLeft, std::set<std::string> &scopesLeft);


private:
    std::set<std::string> getMethodRequiredScopes(const std::string & methodName);
    std::set<std::string> getMethodRequiredRoles(const std::string &methodName);

    // Method -> App Scope
    std::multimap<std::string,std::string> m_methodRequiredScopes;
    std::multimap<std::string,std::string> m_methodRequiredRoles;

};

}}}

