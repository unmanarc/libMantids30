#ifndef IAUTH_VALIDATION_METHODS_H
#define IAUTH_VALIDATION_METHODS_H

#include <map>
#include <string>
#include <set>

#include "iauth_session.h"

namespace CX2 { namespace Authorization { namespace Validation {

class IAuth_Methods_Attributes
{
public:
    IAuth_Methods_Attributes();

    /**
     * @brief addMethodAttributes
     * @param methodName
     * @param attribs
     */
    void addMethodAttributes( const std::string & methodName,  const std::set<std::string> & attribs);
    /**
     * @brief addAttribPassIndexes
     * @param attribName
     * @param passIndexes
     */
    void addAttribPassIndexes( const std::string & attribName,  const std::set<uint32_t> & passIndexes );
    /**
     * @brief validateAttribs Validate account attribs (and if they are authenticated)
     * @param authenticator
     * @param methodName
     * @param passIndexesLeft
     * @return
     */
    bool validateMethod(IAuth *auth, Session::IAuth_Session * authSession, const std::string & methodName, const std::set<uint32_t> &extraTmpIndexes, std::set<uint32_t> * passIndexesLeft, std::set<std::string> *attribsLeft );

    bool getRequireAllMethodsToBeAuthenticated() const;
    void setRequireAllMethodsToBeAuthenticated(bool value);

private:
    std::set<uint32_t> getAttribPassIndexes(const std::string &attribName);
    std::set<std::string> getMethodAttribs(const std::string & methodName);
    std::set<uint32_t> getMethodPassIndexes(const std::string & methodName);

    // Attrib require -> passIndex authentication
    std::multimap<std::string,uint32_t> attribPassIndexes;
    // Method requite ->
    std::multimap<std::string,std::string> methodAttribs;

    bool requireAllMethodsToBeAuthenticated;
};

}}}

#endif // IAUTH_VALIDATION_METHODS_H
