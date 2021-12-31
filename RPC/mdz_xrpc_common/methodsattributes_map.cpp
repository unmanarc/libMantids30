#include "methodsattributes_map.h"

using namespace Mantids::Authentication;

MethodsAttributes_Map::MethodsAttributes_Map()
{
    requireAllMethodsToBeAuthenticated = true;
}

void MethodsAttributes_Map::addMethodAttributes(const std::string &methodName, const std::set<sApplicationAttrib> &attribs)
{
    for (const sApplicationAttrib & attrib : attribs)
        methodAttribs.insert(std::make_pair(methodName, attrib));
}

void MethodsAttributes_Map::addAttribPassIndexes(const sApplicationAttrib &attrib, const std::set<uint32_t> &passIndexes)
{
    for (const uint32_t & passIndex : passIndexes)
        attribPassIndexes.insert(std::make_pair(attrib, passIndex));
}

bool MethodsAttributes_Map::validateMethod(Manager *auth, Session *session,  const std::string &methodName, const std::set<uint32_t> & extraTmpIndexes, std::set<uint32_t> *passIndexesLeft, std::set<sApplicationAttrib> *attribsLeft)
{
    if (auth->isAccountSuperUser(session->getAuthUser()))
    {
        passIndexesLeft->clear();
        attribsLeft->clear();
        return true;
    }

    std::set<uint32_t> requiredPassIndexes  = getMethodPassIndexes(methodName);
    std::set<sApplicationAttrib> requiredAttribs = getMethodAttribs(methodName);

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

    for ( const sApplicationAttrib & attrib : requiredAttribs )
    {
        if (session && auth->accountValidateAttribute(session->getAuthUser(), attrib))
            attribsLeft->erase(attrib);
    }

    return passIndexesLeft->empty() && attribsLeft->empty();
}

std::set<uint32_t> MethodsAttributes_Map::getAttribPassIndexes(const sApplicationAttrib &attrib)
{
    std::set<uint32_t> r = {0};
    auto it = attribPassIndexes.equal_range(attrib);
    for (auto itr = it.first; itr != it.second; ++itr)
        r.insert(itr->second);
    return r;
}

std::set<sApplicationAttrib> MethodsAttributes_Map::getMethodAttribs(const std::string &methodName)
{
    std::set<sApplicationAttrib> r;
    auto it = methodAttribs.equal_range(methodName);
    for (auto itr = it.first; itr != it.second; ++itr)
        r.insert(itr->second);
    return r;
}

std::set<uint32_t> MethodsAttributes_Map::getMethodPassIndexes(const std::string &methodName)
{
    std::set<uint32_t> r;
    std::set<sApplicationAttrib> attribs= getMethodAttribs(methodName);
    for (const sApplicationAttrib & attrib : attribs)
    {
        std::set<uint32_t> passIndexes = getAttribPassIndexes(attrib);

        for (const uint32_t & passIndex : passIndexes )
        {
            r.insert(passIndex);
        }
    }

    if (requireAllMethodsToBeAuthenticated) r.insert(0);

    return r;
}

bool MethodsAttributes_Map::getRequireAllMethodsToBeAuthenticated() const
{
    return requireAllMethodsToBeAuthenticated;
}

void MethodsAttributes_Map::setRequireAllMethodsToBeAuthenticated(bool value)
{
    requireAllMethodsToBeAuthenticated = value;
}
