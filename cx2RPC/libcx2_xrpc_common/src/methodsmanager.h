#ifndef SERVER_METHODS_H
#define SERVER_METHODS_H

#include <map>
#include <list>

#include <json/json.h>

#include <cx2_auth/iauth_domains.h>
#include <cx2_auth/iauth_validation_methods.h>

#include <cx2_thr_threadpool/threadpool.h>
#include <cx2_thr_mutex/mutex_shared.h>

#include "validation_codes.h"

namespace CX2 { namespace RPC { namespace XRPC {
    
struct sRPCParameters
{
    std::string domainName;
    void * rpcMethodsCaller;
    void * connectionSender;
    Authorization::IAuth_Domains *authDomains;
    CX2::Authorization::Session::IAuth_Session *session;
    std::string methodName;
    Json::Value payload;
    Json::Value extraInfo;
    uint64_t requestId;
};

struct sRPCMethod
{
    /**
     * @brief Function pointer.
     */
    Json::Value (*rpcMethod)(void * obj, CX2::Authorization::IAuth *, CX2::Authorization::Session::IAuth_Session * session, const Json::Value & parameters, const Json::Value & extraInfo, Json::Value * extraInfoOut);
    /**
     * @brief obj object to pass
     */
    void * obj;
};

class MethodsManager
{
public:
    MethodsManager(uint32_t threadsCount = 52, uint32_t taskQueues = 36);
    ~MethodsManager();

    void stop();

    /**
     * @brief setTimeout Timeout in milliseconds to desist to put the execution task
     * @param value milliseconds
     */
    void setTimeout(const uint32_t &value);
    /**
     * @brief pushRPCMethodIntoQueue push a received order in the task queue
     * @param params RPC parameters
     * @param key Key to be prioritized
     * @param priority priority (0-1] to use n-queues
     * @return true if inserted, false if failed (saturated)
     */
    bool pushRPCMethodIntoQueue( sRPCParameters * params, const std::string & key, const float & priority=0.5 );

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
    int runRPCMethod(Authorization::IAuth_Domains *, const std::string &domainName, CX2::Authorization::Session::IAuth_Session *auth, const std::string & methodName, const Json::Value & payload, const Json::Value & extraInfo, Json::Value *payloadOut, Json::Value *extraInfoOut);
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
    static void executeRPCTask(void * taskData);


    Json::Value toValue(const std::set<std::string> &t);
    Json::Value toValue(const std::set<uint32_t> &t);

    // Methods:
    // method name -> method.
    std::map<std::string,sRPCMethod> methods;

    CX2::Authorization::Validation::IAuth_Methods_Attributes methodsAttribs;

    CX2::Threads::Pool::ThreadPool * threadPool;
    // lock for methods manipulation...
    Threads::Sync::Mutex_Shared smutexMethods;

    std::atomic<uint32_t> timeout;
};

}}}

#endif // SERVER_METHODS_H
