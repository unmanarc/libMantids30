#ifndef IAUTH_VALIDATION_METHODS_H
#define IAUTH_VALIDATION_METHODS_H

#include <map>
#include <string>
#include <set>

#include <Mantids29/Auth/manager.h>
#include <Mantids29/Auth/session.h>

#include <Mantids29/Auth/accountsecret_validation.h>

namespace Mantids29 { namespace Authentication {

class MethodsAttributes_Map
{
public:
    MethodsAttributes_Map();

    /**
     * @brief addMethodAttributes
     * @param methodName
     * @param attribs
     */
    void addMethodAttributes(const std::string & methodName,  const std::set<ApplicationAttribute> &attribs);
    /**
     * @brief addAttribPassIndexes
     * @param attrib
     * @param passIndexes
     */
    void addAttribPassIndexes(const ApplicationAttribute &attrib,  const std::set<uint32_t> & passIndexes );
    /**
     * @brief validateAttribs Validate account attribs (and if they are authenticated)
     * @param authenticator
     * @param methodName
     * @param passIndexesLeft
     * @return
     */
    bool validateMethod(Manager *auth, Session * authSession, const std::string & methodName, const std::set<uint32_t> &extraTmpIndexes, std::set<uint32_t> * passIndexesLeft, std::set<ApplicationAttribute> *attribsLeft );

    bool m_requireAllMethodsToBeAuthenticated;

private:
    std::set<uint32_t> getAttribPassIndexes(const ApplicationAttribute &attrib);
    std::set<ApplicationAttribute> getMethodAttribs(const std::string & methodName);
    std::set<uint32_t> getMethodPassIndexes(const std::string & methodName);

    // Attrib require -> passIndex authentication
    std::multimap<ApplicationAttribute,uint32_t> m_attribPassIndexes;
    // Method requite ->
    std::multimap<std::string,ApplicationAttribute> m_methodAttribs;

};

}}

#endif // IAUTH_VALIDATION_METHODS_H
