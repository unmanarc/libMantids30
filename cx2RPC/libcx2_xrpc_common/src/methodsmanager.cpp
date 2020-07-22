#include "methodsmanager.h"

#include "request.h"
#include "retcodes.h"
#include "serverbase.h"

#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::RPC::XRPC;
using namespace CX2;

MethodsManager::MethodsManager(uint32_t threadsCount, uint32_t taskQueues)
{
    threadPool = new CX2::Threads::Pool::ThreadPool(threadsCount, taskQueues);
    timeout = 2000; // 2sec.
    threadPool->start();
}

MethodsManager::~MethodsManager()
{
    delete threadPool;
}

void MethodsManager::stop()
{
    threadPool->stop();
}

bool MethodsManager::pushRPCMethodIntoQueue(sRPCParameters *params, const std::string &key, const float &priority)
{
    params->rpcMethodsCaller = this;
    return threadPool->pushTask(executeRPCTask,params,timeout,priority,key);
}


bool MethodsManager::addRPCMethod(const std::string &methodName, const std::set<std::string> &reqAttribs, const sRPCMethod &rpcMethod)
{
    Threads::Sync::Lock_RW lock(smutexMethods);
    if (methods.find(methodName) == methods.end() )
    {
        // Put the method.
        methods[methodName] = rpcMethod;

        // Configure methodsAttribs with this info.
        methodsAttribs.addMethodAttributes(methodName,reqAttribs);

        return true;
    }
    return false;
}

int MethodsManager::runRPCMethod(CX2::Authorization::IAuth_Domains * authDomain, const std::string & domainName,CX2::Authorization::Session::IAuth_Session * session, const std::string & methodName, const Json::Value & payload, const Json::Value & extraInfo, Json::Value *payloadOut, Json::Value *extraInfoOut)
{
    // not authenticated...
    if (session->isAuthenticated()!=Authorization::DataStructs::AUTH_REASON_AUTHENTICATED)
        return METHOD_RET_CODE_UNAUTHENTICATED;

    Threads::Sync::Lock_RD lock(smutexMethods);

    if (methods.find(methodName) == methods.end())
        return METHOD_RET_CODE_METHODNOTFOUND;
    else
    {
        CX2::Authorization::IAuth * auth;
        if ((auth=authDomain->openDomain(domainName))!=nullptr)
        {
            *payloadOut = methods[methodName].rpcMethod(methods[methodName].obj, auth, session,payload,extraInfo,extraInfoOut);
            authDomain->closeDomain(domainName);
            return METHOD_RET_CODE_SUCCESS;
        }
        else
        {
            return METHOD_RET_CODE_INVALIDDOMAIN;
        }

    }
}

eMethodValidationCodes MethodsManager::validateRPCMethodPerms(CX2::Authorization::IAuth * auth, CX2::Authorization::Session::IAuth_Session *session, const std::string &methodName, const std::set<uint32_t> & extraTmpIndexes, Json::Value *reasons)
{
    std::set<uint32_t> passIndexesLeft;
    std::set<std::string> attribsLeft;
    Threads::Sync::Lock_RD lock(smutexMethods);
    if (methods.find(methodName) == methods.end())
    {
        return VALIDATION_METHODNOTFOUND;
    }
    else
    {
        if (methodsAttribs.validateMethod(auth, session,methodName,extraTmpIndexes,&passIndexesLeft,&attribsLeft))
        {
            return VALIDATION_OK;
        }
        else
        {
            // not authorized..
            (*reasons)["passIndexesLeft"] = toValue(passIndexesLeft);
            (*reasons)["attribsLeft"] = toValue(attribsLeft);
            return VALIDATION_NOTAUTHORIZED;
        }
    }
}

CX2::Authorization::Validation::IAuth_Methods_Attributes *MethodsManager::getMethodsAttribs()
{
    return &methodsAttribs;
}

void MethodsManager::executeRPCTask(void *taskData)
{
    sRPCParameters * data = (sRPCParameters *)(taskData);
    Request answer;
    answer.setReqId(data->requestId);
    answer.setMethodName(data->methodName);
    answer.setRpcMode("EXEC");

    Json::Value rPayload, rExtraInfo;
    int rcode;
    rcode = ((MethodsManager *)(data->rpcMethodsCaller))->runRPCMethod(data->authDomains,
                                                                            data->domainName,
                                                                            data->session,
                                                                            data->methodName,
                                                                            data->payload,
                                                                            data->extraInfo,
                                                                            &rPayload,
                                                                            &rExtraInfo);
    answer.setPayload(rPayload);
    answer.setExtraInfo(rExtraInfo);
    answer.setRetCode(rcode);

    ((ServerBase *)(data->connectionSender))->sendAnswer(answer);
    delete data;
}

Json::Value MethodsManager::toValue(const std::set<std::string> &t)
{
    Json::Value x;
    int v=0;
    for (const std::string & i : t)
        x[v++] = i;
    return x;
}

Json::Value MethodsManager::toValue(const std::set<uint32_t> &t)
{
    Json::Value x;
    int v=0;
    for (const uint32_t & i : t)
        x[v++] = i;
    return x;
}

void MethodsManager::setTimeout(const uint32_t &value)
{
    timeout = value;
}
