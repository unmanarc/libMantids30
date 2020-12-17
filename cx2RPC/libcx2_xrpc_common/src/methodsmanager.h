#ifndef SERVER_METHODS_H
#define SERVER_METHODS_H

#include <map>
#include <list>

#include <json/json.h>

#include <cx2_auth/domains.h>
#include <cx2_auth/methodsattributes_map.h>
#include <cx2_thr_mutex/mutex_shared.h>

#include "validation_codes.h"

namespace CX2 { namespace RPC {
    
struct sRPCParameters
{
    std::string domainName;
    void * rpcMethodsCaller;
    void * connectionSender;
    CX2::Authentication::Domains *authDomains;
    CX2::Authentication::Session *session;
    std::string methodName;
    Json::Value payload;
    uint64_t requestId;
};

struct sRPCMethod
{
    /**
     * @brief Function pointer.
     */
    Json::Value (*rpcMethod)(void * obj, CX2::Authentication::Manager *, CX2::Authentication::Session * session, const Json::Value & parameters);
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
    int runRPCMethod(CX2::Authentication::Domains *, const std::string &domainName, CX2::Authentication::Session *auth, const std::string & methodName, const Json::Value & payload, Json::Value *payloadOut);
    /**
     * @brief validateRPCMethod
     * @param auth
     * @param methodName
     * @param payloadOut
     * @param extraInfoOut
     * @return
     */
    eMethodValidationCodes validateRPCMethodPerms(Authentication::Manager *auth, CX2::Authentication::Session *session, const std::string & methodName, const std::set<uint32_t> &extraTmpIndexes, Json::Value * reasons);

    /**
     * @brief getMethodsAttribs Use for method initialization only.
     * @return methods required attributes
     */
    CX2::Authentication::MethodsAttributes_Map * getMethodsAttribs();


private:
    Json::Value toValue(const std::set<std::string> &t);
    Json::Value toValue(const std::set<uint32_t> &t);

    // Methods:
    // method name -> method.
    std::map<std::string,sRPCMethod> methods;

    CX2::Authentication::MethodsAttributes_Map methodsAttribs;

    // lock for methods manipulation...
    Threads::Sync::Mutex_Shared smutexMethods;
};

}}

#endif // SERVER_METHODS_H
