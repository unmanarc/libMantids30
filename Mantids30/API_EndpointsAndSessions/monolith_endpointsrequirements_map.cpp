#include "monolith_endpointsrequirements_map.h"
#include <memory>
#include <string>

using namespace Mantids30::API::Monolith;

void EndpointsRequirements_Map::addEndpointRequiredScopes(const std::string &endpointName, const std::set<std::string> &applicationScopes)
{
    for (const std::string & scope: applicationScopes)
        m_endpointRequiredScopes.insert(std::make_pair(endpointName, scope));
}

std::set<std::string> EndpointsRequirements_Map::getEndpointRequiredScopes(const std::string &endpointName)
{
    std::set<std::string> r;
    auto it = m_endpointRequiredScopes.equal_range(endpointName);
    for (auto itr = it.first; itr != it.second; ++itr)
        r.insert(itr->second);
    return r;
}

void EndpointsRequirements_Map::addEndpointRequiredRoles(const std::string &endpointName, const std::set<std::string> &applicationRoles)
{
    for (const std::string & role : applicationRoles)
        m_endpointRequiredRoles.insert(std::make_pair(endpointName, role));
}

std::set<std::string> EndpointsRequirements_Map::getEndpointRequiredRoles(const std::string &endpointName)
{
    std::set<std::string> r;
    auto it = m_endpointRequiredRoles.equal_range(endpointName);
    for (auto itr = it.first; itr != it.second; ++itr)
        r.insert(itr->second);
    return r;
}

bool EndpointsRequirements_Map::validateEndpoint(std::shared_ptr<Sessions::Session> session, const std::string &endpointName, std::set<std::string> &rolesLeft, std::set<std::string> &scopesLeft)
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

    std::set<std::string> requiredRoles  = getEndpointRequiredRoles(endpointName);
    std::set<std::string> requiredScopes = getEndpointRequiredScopes(endpointName);

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

