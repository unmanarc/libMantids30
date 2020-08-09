#ifndef SERVER_METHODS_H
#define SERVER_METHODS_H

#include <map>
#include <list>

#include <json/json.h>

#include <cx2_auth/iauth_domains.h>
#include <cx2_auth/iauth_methods_attributes.h>
#include <cx2_thr_mutex/mutex_shared.h>

#include "validation_codes.h"

namespace CX2 { namespace RPC {
    
struct sRPCParameters
{
    std::string domainName;
    void * rpcMethodsCaller;
    void * connectionSender;
    Authorization::IAuth_Domains *authDomains;
    CX2::Authorization::Session::IAuth_Session *session;
    std::string methodName;
    Json::Value payload;
//    Json::Value extraInfo;
    uint64_t requestId;
};

struct sRPCMethod
{
    /**
     * @brief Function pointer.
     */
    Json::Value (*rpcMethod)(void * obj, CX2::Authorization::IAuth *, CX2::Authorization::Session::IAuth_Session * session, const Json::Value & parameters);
    /**
     * @brief obj object to pass
     */
    void * obj;
};

class MethodsManager
{
public:
    MethodsManager();

    //////////////////////////////////////////////////
    /**
     * @brief addRPCMethod
     * @param methodName
     * @param reqAttribs
     * @param rpcMethod
     * @return
     */
    bool addRPCMethod(const std::string & methodName, const std::set<std::string> & reqAttribs, const sRPCMethod & rpcMethod);
    /**
     * @brief runRPCMethod2
     * @param methodName
     * @param parsedParams
     * @param extraInfo
     * @param answer
     * @return 0 if succeed, -4 if method not found.
     */
    int runRPCMethod(Authorization::IAuth_Domains *, const std::string &domainName, CX2::Authorization::Session::IAuth_Session *auth, const std::string & methodName, const Json::Value & payload, Json::Value *payloadOut);
    /**
     * @brief validateRPCMethod
     * @param auth
     * @param methodName
     * @param payloadOut
     * @param extraInfoOut
     * @return
     */
    eMethodValidationCodes validateRPCMethodPerms(Authorization::IAuth *auth, CX2::Authorization::Session::IAuth_Session *session, const std::string & methodName, const std::set<uint32_t> &extraTmpIndexes, Json::Value * reasons);

    /**
     * @brief getMethodsAttribs Use for method initialization only.
     * @return methods required attributes
     */
    CX2::Authorization::Validation::IAuth_Methods_Attributes * getMethodsAttribs();


private:
    Json::Value toValue(const std::set<std::string> &t);
    Json::Value toValue(const std::set<uint32_t> &t);

    // Methods:
    // method name -> method.
    std::map<std::string,sRPCMethod> methods;

    CX2::Authorization::Validation::IAuth_Methods_Attributes methodsAttribs;

    // lock for methods manipulation...
    Threads::Sync::Mutex_Shared smutexMethods;
};

}}

#endif // SERVER_METHODS_H
