#include "endpoints_requirements_map.h"
#include <memory>
#include <string>

using namespace Mantids30::API;

void EndpointsRequirements_Map::addEndpointRequiredScopes(const std::string &endpointName, const std::set<std::string> &applicationScopes)
{
    for (const std::string &scope : applicationScopes)
    {
        m_endpointRequiredScopes.insert(std::make_pair(endpointName, scope));
    }
}

std::set<std::string> EndpointsRequirements_Map::getEndpointRequiredScopes(const std::string &endpointName)
{
    std::set<std::string> r;
    std::pair<std::multimap<std::string, std::string>::iterator, std::multimap<std::string, std::string>::iterator> it = m_endpointRequiredScopes.equal_range(endpointName);
    for (std::multimap<std::string, std::string>::iterator itr = it.first; itr != it.second; ++itr)
    {
        r.insert(itr->second);
    }
    return r;
}

void EndpointsRequirements_Map::addEndpointRequiredRoles(const std::string &endpointName, const std::set<std::string> &applicationRoles)
{
    for (const std::string &role : applicationRoles)
    {
        m_endpointRequiredRoles.insert(std::make_pair(endpointName, role));
    }
}

std::set<std::string> EndpointsRequirements_Map::getEndpointRequiredRoles(const std::string &endpointName)
{
    std::set<std::string> r;
    std::pair<std::multimap<std::string, std::string>::iterator, std::multimap<std::string, std::string>::iterator> it = m_endpointRequiredRoles.equal_range(endpointName);
    for (std::multimap<std::string, std::string>::iterator itr = it.first; itr != it.second; ++itr)
    {
        r.insert(itr->second);
    }
    return r;
}

bool EndpointsRequirements_Map::validateEndpoint(const std::shared_ptr<Sessions::Session> &session, const std::string &endpointName, std::set<std::string> &rolesLeft, std::set<std::string> &scopesLeft)
{
    if (!session)
    {
        return false;
    }

    const auto &authInfo = session->getJWTAuthenticatedInfo();

    if (authInfo.isAdmin())
    {
        rolesLeft.clear();
        scopesLeft.clear();
        return true;
    }

    rolesLeft = getEndpointRequiredRoles(endpointName);
    scopesLeft = getEndpointRequiredScopes(endpointName);

    for (auto it = rolesLeft.begin(); it != rolesLeft.end();)
    {
        if (authInfo.hasRole(*it))
        {
            it = rolesLeft.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto it = scopesLeft.begin(); it != scopesLeft.end();)
    {
        if (authInfo.hasScope(*it))
        {
            it = scopesLeft.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return rolesLeft.empty() && scopesLeft.empty();
}
