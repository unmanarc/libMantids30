#ifndef SERVER_METHODS_H
#define SERVER_METHODS_H

#include <map>
#include <list>

#include <mdz_hlp_functions/json.h>

#include <mdz_auth/domains.h>
#include <mdz_thr_mutex/mutex_shared.h>

#include "validation_codes.h"
#include "methodsattributes_map.h"

namespace Mantids { namespace RPC {
    
struct sRPCParameters
{
    std::string domainName;
    void * rpcMethodsCaller;
    void * connectionSender;
    Mantids::Authentication::Domains *authDomains;
    Mantids::Authentication::Session *session;
    std::string methodName;
    json payload;
    uint64_t requestId;
};

struct sRPCMethod
{
    /**
     * @brief Function pointer.
     */
    json (*rpcMethod)(void * obj, Mantids::Authentication::Manager *, Mantids::Authentication::Session * session, const json & parameters);
    /**
     * @brief obj object to pass
     */
    void * obj;
};

class MethodsManager
{
public:
    /**
     * @brief MethodsManager Constructor
     * @param appName Application Name
     */
    MethodsManager(const std::string & appName);

    //////////////////////////////////////////////////

    /**
     * @brief addRPCMethod Add New RPC Method
     * @param methodName Method Name (the name to be used in the rpc calls)
     * @param reqAttribs Required Attributes from the authenticator
     * @param rpcMethod RPC Method definition (struct) which include the function and one generic object pointer
     * @param requireFullAuth true if the method requires full user authentication, false for public methods.
     * @return true if the method was inserted.
     */
    bool addRPCMethod(const std::string & methodName, const std::set<std::string> & reqAttribs, const sRPCMethod & rpcMethod, bool requireFullAuth = true);
    /**
     * @brief runRPCMethod Run RPC Method
     * @param authDomain Authentication Domain Pool
     * @param domainName Domain to be used
     * @param auth Session Authentication
     * @param methodName Method to be executed
     * @param payload Payload for the method
     * @param payloadOut Response Payload (out)
     * @return 0 if succeed, -4 if method not found.
     */
    int runRPCMethod(Mantids::Authentication::Domains *authDomain, const std::string &domainName, Mantids::Authentication::Session *auth, const std::string & methodName, const json & payload, json *payloadOut);
    /**
     * @brief validateRPCMethodPerms Validate that RPC Method is authorized
     * @param auth Authentication Manager
     * @param session Session Authentication
     * @param methodName Method to be validated
     * @param extraTmpIndexes Extra Temporary Indexes
     * @param reasons Fancy Response
     * @return VALIDATION_OK/VALIDATION_METHODNOTFOUND/VALIDATION_NOTAUTHORIZED codes.
     */
    eMethodValidationCodes validateRPCMethodPerms(Authentication::Manager *auth, Mantids::Authentication::Session *session, const std::string & methodName, const std::set<uint32_t> &extraTmpIndexes, json * reasons);
    /**
     * @brief getMethodsAttribs Use for method initialization only.
     * @return methods required attributes
     */
    Mantids::Authentication::MethodsAttributes_Map * getMethodsAttribs();

    /**
     * @brief getMethodRequireFullAuth Get if a method requires full authentication
     * @param methodName Method Name
     * @return true if requires full session/authentication
     */
    bool getMethodRequireFullAuth(const std::string & methodName);
    /**
     * @brief getAppName Get Current Application Name
     * @return Application Name String
     */
    std::string getAppName() const;

private:
    std::set<Mantids::Authentication::sApplicationAttrib> getAppAttribs(const std::set<std::string> & reqAttribs);

    json toValue(const std::set<Mantids::Authentication::sApplicationAttrib> &t);

    // TODO: move to some helper
    json toValue(const std::set<std::string> &t);
    json toValue(const std::set<uint32_t> &t);

    // Methods:

    // method name -> method.
    std::map<std::string,sRPCMethod> methods;

    // method name -> bool (requireFullAuth).
    std::map<std::string,bool> methodRequireFullAuth;

    std::string appName;
    Mantids::Authentication::MethodsAttributes_Map methodsAttribs;

    // lock for methods manipulation...
    Threads::Sync::Mutex_Shared smutexMethods;
};

}}

#endif // SERVER_METHODS_H
