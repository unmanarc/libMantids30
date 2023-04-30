#include "methodshandler.h"
#include "Mantids29/Protocol_HTTP/rsp_status.h"
#include <Mantids29/Threads/lock_shared.h>

using namespace Mantids29;
using namespace API::RESTful;

MethodsHandler::MethodsHandler()
{

}

bool MethodsHandler::addResource(const MethodMode &mode, const std::string &resourceName, APIReturn (*method)(void *, const RESTful::InputParameters &), void *obj, bool requireUserAuthentication, const std::set<std::string> requiredAttributes)
{
    RESTfulAPIDefinition def;
    def.method = method;
    def.obj = obj;
    def.security.requireUserAuthentication = requireUserAuthentication;
    def.security.requiredAttributes = requiredAttributes;
    return addResource(mode, resourceName, def);
}

bool MethodsHandler::addResource(const MethodMode &mode, const std::string &resourceName, const RESTfulAPIDefinition &method)
{
    Threads::Sync::Lock_RW lock(m_methodsMutex);

    switch (mode)
    {
    case GET:
        m_methodsGET[resourceName] = method;
        break;
    case POST:
        m_methodsPOST[resourceName] = method;
        break;
    case PUT:
        m_methodsPUT[resourceName] = method;
        break;
    case DELETE:
        m_methodsDELETE[resourceName] = method;
        break;
    default:
        return false;
    }

    return true;
}

MethodsHandler::ErrorCodes MethodsHandler::invokeResource(const MethodMode & mode, const std::string & resourceName, const RESTful::InputParameters &inputParameters, const std::set<std::string> &currentAttributes, bool authenticated, APIReturn *payloadOut)
{    Threads::Sync::Lock_RD lock(m_methodsMutex);

    RESTfulAPIDefinition method;
    auto it = m_methodsGET.end();

    switch (mode)
    {
    case GET:
        it = m_methodsGET.find(resourceName);
        if (it != m_methodsGET.end())
        {
            method = it->second;
        }
        break;
    case POST:
        it = m_methodsPOST.find(resourceName);
        if (it != m_methodsPOST.end())
        {
            method = it->second;
        }
        break;
    case PUT:
        it = m_methodsPUT.find(resourceName);
        if (it != m_methodsPUT.end())
        {
            method = it->second;
        }
        break;
    case DELETE:
        it = m_methodsDELETE.find(resourceName);
        if (it != m_methodsDELETE.end())
        {
            method = it->second;
        }
        break;
    default:
        if (payloadOut != nullptr)
        {
            *payloadOut->body = "Invalid method mode";
            payloadOut->code = Network::Protocols::HTTP::Status::S_400_BAD_REQUEST;
        }
        return INVALID_METHOD_MODE;
    }

    if (it == m_methodsGET.end())
    {
        if (payloadOut != nullptr)
        {
            *payloadOut->body = "Resource not found";
            payloadOut->code = Network::Protocols::HTTP::Status::S_404_NOT_FOUND;
        }
        return RESOURCE_NOT_FOUND;
    }

    if (method.security.requireUserAuthentication && !authenticated)
    {
        if (payloadOut != nullptr)
        {
            *payloadOut->body = "Authentication required";
            payloadOut->code = Network::Protocols::HTTP::Status::S_403_FORBIDDEN;
        }
        return AUTHENTICATION_REQUIRED;
    }

    for (const auto &attr : method.security.requiredAttributes)
    {
        if (currentAttributes.find(attr) == currentAttributes.end())
        {
            if (payloadOut != nullptr)
            {
                *payloadOut->body = "Insufficient permissions";
                payloadOut->code = Network::Protocols::HTTP::Status::S_401_UNAUTHORIZED;
            }
            return INSUFFICIENT_PERMISSIONS;
        }
    }

    if (method.method != nullptr && payloadOut != nullptr)
    {
        *payloadOut = method.method(method.obj, inputParameters);
        return SUCCESS;
    }

    if (payloadOut != nullptr)
    {
        *payloadOut->body = "Internal error";
        payloadOut->code = Network::Protocols::HTTP::Status::S_500_INTERNAL_SERVER_ERROR;
    }

    return INTERNAL_ERROR;
}

MethodsHandler::ErrorCodes MethodsHandler::invokeResource(const std::string &modeStr, const std::string &resourceName, const InputParameters &inputParameters, const std::set<std::string> &currentAttributes, bool authenticated, APIReturn *payloadOut)
{
    MethodMode mode;

    if (modeStr == "GET")
    {
        mode = GET;
    }
    else if (modeStr == "POST")
    {
        mode = POST;
    }
    else if (modeStr == "PUT")
    {
        mode = PUT;
    }
    else if (modeStr == "DELETE")
    {
        mode = DELETE;
    }
    else
    {
        if (payloadOut != nullptr)
        {
            *payloadOut->body = "Invalid method mode string";
            payloadOut->code = Network::Protocols::HTTP::Status::S_400_BAD_REQUEST;
        }
        return INVALID_METHOD_MODE;
    }

    return invokeResource(mode, resourceName, inputParameters, currentAttributes, authenticated, payloadOut);

}

