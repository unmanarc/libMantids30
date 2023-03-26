#include "methodsattributes_map.h"

using namespace Mantids29::Authentication;

MethodsAttributes_Map::MethodsAttributes_Map()
{
    m_requireAllMethodsToBeAuthenticated = true;
}

void MethodsAttributes_Map::addMethodAttributes(const std::string &methodName, const std::set<ApplicationAttribute> &attribs)
{
    for (const ApplicationAttribute & attrib : attribs)
        m_methodAttribs.insert(std::make_pair(methodName, attrib));
}

void MethodsAttributes_Map::addAttribPassIndexes(const ApplicationAttribute &attrib, const std::set<uint32_t> &passIndexes)
{
    for (const uint32_t & passIndex : passIndexes)
        m_attribPassIndexes.insert(std::make_pair(attrib, passIndex));
}

bool MethodsAttributes_Map::validateMethod(Manager *auth, Session *session,  const std::string &methodName, const std::set<uint32_t> & extraTmpIndexes, std::set<uint32_t> *passIndexesLeft, std::set<ApplicationAttribute> *attribsLeft)
{
    if (auth->isAccountSuperUser(session->getAuthUser()))
    {
        passIndexesLeft->clear();
        attribsLeft->clear();
        return true;
    }

    std::set<uint32_t> requiredPassIndexes  = getMethodPassIndexes(methodName);
    std::set<ApplicationAttribute> requiredAttribs = getMethodAttribs(methodName);

    // start with all required attribs/passIndexes...
    *passIndexesLeft = requiredPassIndexes;
    *attribsLeft = requiredAttribs;

    // Validate the required pass indexes for attribs/method
    for ( const uint32_t & passIndex : requiredPassIndexes )
    {
        // Check if the session provides the required pass index authenticated.
        if ( session && IS_PASSWORD_AUTHENTICATED(session->getIdxAuthenticationStatus(passIndex)) )
        {
            passIndexesLeft->erase(passIndex);
        }
        if ( extraTmpIndexes.find(passIndex) != extraTmpIndexes.end() )
        {
            passIndexesLeft->erase(passIndex);
        }
    }

    for ( const ApplicationAttribute & attrib : requiredAttribs )
    {
        if (session && auth->validateAccountAttribute(session->getAuthUser(), attrib))
            attribsLeft->erase(attrib);
    }

    return passIndexesLeft->empty() && attribsLeft->empty();
}

std::set<uint32_t> MethodsAttributes_Map::getAttribPassIndexes(const ApplicationAttribute &attrib)
{
    std::set<uint32_t> r = {0};
    auto it = m_attribPassIndexes.equal_range(attrib);
    for (auto itr = it.first; itr != it.second; ++itr)
        r.insert(itr->second);
    return r;
}

std::set<ApplicationAttribute> MethodsAttributes_Map::getMethodAttribs(const std::string &methodName)
{
    std::set<ApplicationAttribute> r;
    auto it = m_methodAttribs.equal_range(methodName);
    for (auto itr = it.first; itr != it.second; ++itr)
        r.insert(itr->second);
    return r;
}

std::set<uint32_t> MethodsAttributes_Map::getMethodPassIndexes(const std::string &methodName)
{
    std::set<uint32_t> r;
    std::set<ApplicationAttribute> attribs= getMethodAttribs(methodName);
    for (const ApplicationAttribute & attrib : attribs)
    {
        std::set<uint32_t> passIndexes = getAttribPassIndexes(attrib);

        for (const uint32_t & passIndex : passIndexes )
        {
            r.insert(passIndex);
        }
    }

    if (m_requireAllMethodsToBeAuthenticated) r.insert(0);

    return r;
}


