#include "methodsmanager.h"

#include "multiauths.h"
#include "retcodes.h"

#include <mdz_thr_mutex/lock_shared.h>

using namespace Mantids::RPC;
using namespace Mantids;

MethodsManager::MethodsManager(const std::string &appName)
{
    this->appName = appName;
}

bool MethodsManager::addRPCMethod(const std::string &methodName, const std::set<std::string> &reqAttribs, const sRPCMethod &rpcMethod, bool requireFullAuth)
{
    Threads::Sync::Lock_RW lock(smutexMethods);
    if (methods.find(methodName) == methods.end() )
    {
        // Put the method.
        methods[methodName] = rpcMethod;

        // Configure methodsAttribs with this info.
        methodsAttribs.addMethodAttributes(methodName,getAppAttribs(reqAttribs));

        methodRequireFullAuth[methodName] = requireFullAuth;

        return true;
    }
    return false;
}

int MethodsManager::runRPCMethod(Mantids::Authentication::Domains * authDomain, const std::string & domainName,Mantids::Authentication::Session * session, const std::string & methodName, const json & payload,  json *payloadOut)
{
    Threads::Sync::Lock_RD lock(smutexMethods);

    if (methods.find(methodName) == methods.end())
        return METHOD_RET_CODE_METHODNOTFOUND;
    else
    {
        Mantids::Authentication::Manager * auth;
        if ((auth=authDomain->openDomain(domainName))!=nullptr)
        {
            *payloadOut = methods[methodName].rpcMethod(methods[methodName].obj, auth, session,payload);
            authDomain->releaseDomain(domainName);
            return METHOD_RET_CODE_SUCCESS;
        }
        else
        {
            return METHOD_RET_CODE_INVALIDDOMAIN;
        }

    }
}

eMethodValidationCodes MethodsManager::validateRPCMethodPerms(Mantids::Authentication::Manager * auth, Mantids::Authentication::Session *session, const std::string &methodName, const std::set<uint32_t> & extraTmpIndexes, json *reasons)
{
    std::set<uint32_t> passIndexesLeft;
    std::set<Mantids::Authentication::sApplicationAttrib> attribsLeft;
    Threads::Sync::Lock_RD lock(smutexMethods);

    // Check if the method exist at all:
    if (methods.find(methodName) == methods.end())
        return VALIDATION_METHODNOTFOUND;

    // If requires full authentication, check that the session report that is fully authenticated (all required ID's) and it's also a persistent session.
    if (methodRequireFullAuth[methodName])
    {
        if (!session || !session->getIsFullyLoggedIn(Mantids::Authentication::CHECK_DISALLOW_EXPIRED_PASSWORDS) || !session->getIsPersistentSession())
            return VALIDATION_NOTAUTHORIZED;
    }
    // else: otherwise, the method will only be validated against authenticated attribs/indexes

    // Validate that the method haves the required attribs/pass indexes:
    if (methodsAttribs.validateMethod(auth,session,methodName,extraTmpIndexes,&passIndexesLeft,&attribsLeft))
    {
        return VALIDATION_OK;
    }
    else
    {
        // The method is not authorized for this authentication level.. Report what is failing.
        (*reasons)["passIndexesLeft"] = toValue(passIndexesLeft);
        (*reasons)["attribsLeft"] = toValue(attribsLeft);
        return VALIDATION_NOTAUTHORIZED;
    }

}

Mantids::Authentication::MethodsAttributes_Map *MethodsManager::getMethodsAttribs()
{
    return &methodsAttribs;
}

bool MethodsManager::getMethodRequireFullAuth(const std::string &methodName)
{
    Threads::Sync::Lock_RD lock(smutexMethods);
    return methodRequireFullAuth[methodName];
}

std::set<Mantids::Authentication::sApplicationAttrib> MethodsManager::getAppAttribs(const std::set<std::string> &reqAttribs)
{
    std::set<Mantids::Authentication::sApplicationAttrib> r;
    for (const auto &i : reqAttribs)
    {
        r.insert({appName,i});
    }
    return r;
}

json MethodsManager::toValue(const std::set<Mantids::Authentication::sApplicationAttrib> &t)
{
    json x;
    int v=0;
    for (const auto &i : t)
    {
        x[v++] = i.attribName;
    }
    return x;
}

json MethodsManager::toValue(const std::set<std::string> &t)
{
    json x;
    int v=0;
    for (const std::string & i : t)
        x[v++] = i;
    return x;
}

json MethodsManager::toValue(const std::set<uint32_t> &t)
{
    json x;
    int v=0;
    for (const uint32_t & i : t)
        x[v++] = i;
    return x;
}

std::string MethodsManager::getAppName() const
{
    return appName;
}
