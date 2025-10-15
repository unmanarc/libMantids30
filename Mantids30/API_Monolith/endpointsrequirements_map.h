#pragma once

#include <map>
#include <string>
#include <set>

#include <Mantids30/Sessions/session.h>

namespace Mantids30 { namespace API { namespace Monolith {

class EndpointsRequirements_Map
{
public:
    EndpointsRequirements_Map() = default;

    /**
     * @brief addEndpointRequiredScopes
     * @param endpointName
     * @param applicationScopes
     */
    void addEndpointRequiredScopes(const std::string &endpointName, const std::set<std::string> &applicationScopes);

    void addEndpointRequiredRoles(const std::string &endpointName, const std::set<std::string> &applicationRoles);


    /**
     * @brief validateScopes Validate account application scopes (and if they are authenticated)
     * @param authenticator
     * @param endpointName
     * @param slotIdsLeft
     * @return
     */
    bool validateEndpoint(std::shared_ptr<Sessions::Session> authSession, const std::string &endpointName, std::set<std::string> &rolesLeft, std::set<std::string> &scopesLeft);


private:
    std::set<std::string> getEndpointRequiredScopes(const std::string & endpointName);
    std::set<std::string> getEndpointRequiredRoles(const std::string &endpointName);

    // Endpoint -> App Scope
    std::multimap<std::string,std::string> m_endpointRequiredScopes;
    // Endpoint -> App Role
    std::multimap<std::string,std::string> m_endpointRequiredRoles;

};

}}}

