#include "methodshandler.h"
#include <Mantids29/Threads/lock_shared.h>

using namespace Mantids29::Network::API;
using namespace Mantids29;

MethodsHandler::MethodsHandler(const std::string &appName)
{
    this->appName = appName;
}

bool MethodsHandler::addMethod(const std::string &methodName, const std::set<std::string> &reqAttribs, const sRPCMethod &rpcMethod, bool requireFullAuth)
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

int MethodsHandler::invoke(Mantids29::Authentication::Domains * authDomain, const std::string & domainName,Mantids29::Authentication::Session * session, const std::string & methodName, const json & payload,  json *payloadOut)
{
    Threads::Sync::Lock_RD lock(smutexMethods);

    if (methods.find(methodName) == methods.end())
        return METHOD_RET_CODE_METHODNOTFOUND;
    else
    {
        Mantids29::Authentication::Manager * auth;
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

MethodsHandler::eMethodValidationCodes MethodsHandler::validatePermissions(Mantids29::Authentication::Manager * auth, Mantids29::Authentication::Session *session, const std::string &methodName, const std::set<uint32_t> & extraTmpIndexes, json *reasons)
{
    std::set<uint32_t> passIndexesLeft;
    std::set<Mantids29::Authentication::ApplicationAttribute> attribsLeft;
    Threads::Sync::Lock_RD lock(smutexMethods);

    // Check if the method exist at all:
    if (methods.find(methodName) == methods.end())
        return VALIDATION_METHODNOTFOUND;

    // If requires full authentication, check that the session report that is fully authenticated (all required ID's) and it's also a persistent session.
    if (methodRequireFullAuth[methodName])
    {
        if (!session || !session->isFullyAuthenticated(Mantids29::Authentication::Session::CHECK_DISALLOW_EXPIRED_PASSWORDS) || !session->isPersistentSession())
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

Mantids29::Authentication::MethodsAttributes_Map *MethodsHandler::getMethodsAttribs()
{
    return &methodsAttribs;
}

bool MethodsHandler::getMethodRequireFullAuth(const std::string &methodName)
{
    Threads::Sync::Lock_RD lock(smutexMethods);
    return methodRequireFullAuth[methodName];
}

std::set<Mantids29::Authentication::ApplicationAttribute> MethodsHandler::getAppAttribs(const std::set<std::string> &reqAttribs)
{
    std::set<Mantids29::Authentication::ApplicationAttribute> r;
    for (const auto &i : reqAttribs)
    {
        r.insert({appName,i});
    }
    return r;
}

json MethodsHandler::toValue(const std::set<Mantids29::Authentication::ApplicationAttribute> &t)
{
    json x;
    int v=0;
    for (const auto &i : t)
    {
        x[v++] = i.attribName;
    }
    return x;
}

json MethodsHandler::toValue(const std::set<std::string> &t)
{
    json x;
    int v=0;
    for (const std::string & i : t)
        x[v++] = i;
    return x;
}

json MethodsHandler::toValue(const std::set<uint32_t> &t)
{
    json x;
    int v=0;
    for (const uint32_t & i : t)
        x[v++] = i;
    return x;
}

std::string MethodsHandler::getApplicationName() const
{
    return appName;
}
