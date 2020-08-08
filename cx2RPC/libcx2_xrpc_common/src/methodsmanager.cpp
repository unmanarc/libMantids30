#include "methodsmanager.h"

#include "request.h"
#include "retcodes.h"

#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::RPC;
using namespace CX2;

MethodsManager::MethodsManager()
{
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

int MethodsManager::runRPCMethod(CX2::Authorization::IAuth_Domains * authDomain, const std::string & domainName,CX2::Authorization::Session::IAuth_Session * session, const std::string & methodName, const Json::Value & payload,  Json::Value *payloadOut)
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
            *payloadOut = methods[methodName].rpcMethod(methods[methodName].obj, auth, session,payload);
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
