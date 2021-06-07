#ifndef SERVER_METHODS_H
#define SERVER_METHODS_H

#include <map>
#include <list>

#include <cx2_hlp_functions/json.h>

#include <cx2_auth/domains.h>
#include <cx2_thr_mutex/mutex_shared.h>

#include "validation_codes.h"
#include "methodsattributes_map.h"

namespace CX2 { namespace RPC {
    
struct sRPCParameters
{
    std::string domainName;
    void * rpcMethodsCaller;
    void * connectionSender;
    CX2::Authentication::Domains *authDomains;
    CX2::Authentication::Session *session;
    std::string methodName;
    json payload;
    uint64_t requestId;
};

struct sRPCMethod
{
    /**
     * @brief Function pointer.
     */
    json (*rpcMethod)(void * obj, CX2::Authentication::Manager *, CX2::Authentication::Session * session, const json & parameters);
    /**
     * @brief obj object to pass
     */
    void * obj;
};

class MethodsManager
{
public:
    MethodsManager(const std::string & appName);

    //////////////////////////////////////////////////
    /**
     * @brief addRPCMethod
     * @param methodName
     * @param reqAttribs
     * @param rpcMethod
     * @return
     */
    bool addRPCMethod(const std::string & methodName, const std::set<std::string> & reqAttribs, const sRPCMethod & rpcMethod, bool requireFullAuth = true);
    /**
     * @brief runRPCMethod2
     * @param methodName
     * @param parsedParams
     * @param extraInfo
     * @param answer
     * @return 0 if succeed, -4 if method not found.
     */
    int runRPCMethod(CX2::Authentication::Domains *, const std::string &domainName, CX2::Authentication::Session *auth, const std::string & methodName, const json & payload, json *payloadOut);
    /**
     * @brief validateRPCMethod
     * @param auth
     * @param methodName
     * @param payloadOut
     * @param extraInfoOut
     * @return
     */
    eMethodValidationCodes validateRPCMethodPerms(Authentication::Manager *auth, CX2::Authentication::Session *session, const std::string & methodName, const std::set<uint32_t> &extraTmpIndexes, json * reasons);
    /**
     * @brief getMethodsAttribs Use for method initialization only.
     * @return methods required attributes
     */
    CX2::Authentication::MethodsAttributes_Map * getMethodsAttribs();

    bool getMethodRequireFullSession(const std::string & methodName);

    /**
     * @brief getAppName Get Application Name
     * @return app name
     */
    std::string getAppName() const;

private:
    std::set<CX2::Authentication::sApplicationAttrib> getAppAttribs(const std::set<std::string> & reqAttribs);

    json toValue(const std::set<CX2::Authentication::sApplicationAttrib> &t);
    json toValue(const std::set<std::string> &t);
    json toValue(const std::set<uint32_t> &t);

    // Methods:

    // method name -> method.
    std::map<std::string,sRPCMethod> methods;

    // method name -> bool (requireFullAuth).
    std::map<std::string,bool> methodRequireFullAuth;

    std::string appName;
    CX2::Authentication::MethodsAttributes_Map methodsAttribs;

    // lock for methods manipulation...
    Threads::Sync::Mutex_Shared smutexMethods;
};

}}

#endif // SERVER_METHODS_H
