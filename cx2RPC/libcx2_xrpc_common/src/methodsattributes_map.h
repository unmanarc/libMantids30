#ifndef IAUTH_VALIDATION_METHODS_H
#define IAUTH_VALIDATION_METHODS_H

#include <map>
#include <string>
#include <set>

#include <cx2_auth/manager.h>
#include <cx2_auth/session.h>

#include <cx2_auth/accountsecret_validation.h>

namespace CX2 { namespace Authentication {

class MethodsAttributes_Map
{
public:
    MethodsAttributes_Map();

    /**
     * @brief addMethodAttributes
     * @param methodName
     * @param attribs
     */
    void addMethodAttributes(const std::string & methodName,  const std::set<sApplicationAttrib> &attribs);
    /**
     * @brief addAttribPassIndexes
     * @param attrib
     * @param passIndexes
     */
    void addAttribPassIndexes(const sApplicationAttrib &attrib,  const std::set<uint32_t> & passIndexes );
    /**
     * @brief validateAttribs Validate account attribs (and if they are authenticated)
     * @param authenticator
     * @param methodName
     * @param passIndexesLeft
     * @return
     */
    bool validateMethod(Manager *auth, Session * authSession, const std::string & methodName, const std::set<uint32_t> &extraTmpIndexes, std::set<uint32_t> * passIndexesLeft, std::set<sApplicationAttrib> *attribsLeft );

    bool getRequireAllMethodsToBeAuthenticated() const;
    void setRequireAllMethodsToBeAuthenticated(bool value);

private:
    std::set<uint32_t> getAttribPassIndexes(const sApplicationAttrib &attrib);
    std::set<sApplicationAttrib> getMethodAttribs(const std::string & methodName);
    std::set<uint32_t> getMethodPassIndexes(const std::string & methodName);

    // Attrib require -> passIndex authentication
    std::multimap<sApplicationAttrib,uint32_t> attribPassIndexes;
    // Method requite ->
    std::multimap<std::string,sApplicationAttrib> methodAttribs;

    bool requireAllMethodsToBeAuthenticated;
};

}}

#endif // IAUTH_VALIDATION_METHODS_H
