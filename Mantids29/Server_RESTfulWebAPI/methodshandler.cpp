#include "methodshandler.h"
#include <Mantids29/Threads/lock_shared.h>

using namespace Mantids29;
using namespace Network::Servers::RESTful;

MethodsHandler::MethodsHandler()
{

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

MethodsHandler::ErrorCodes MethodsHandler::invokeResource(const MethodMode & mode, const std::string & resourceName, const RESTful::Parameters &inputParameters, const std::set<std::string> &currentAttributes, bool authenticated, Json::Value *payloadOut) {
    Threads::Sync::Lock_RD lock(m_methodsMutex);

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
            *payloadOut = "Invalid method mode";
        }
        return INVALID_METHOD_MODE;
    }

    if (it == m_methodsGET.end())
    {
        if (payloadOut != nullptr)
        {
            *payloadOut = "Resource not found";
        }
        return RESOURCE_NOT_FOUND;
    }

    if (method.security.requireUserAuthentication && !authenticated)
    {
        if (payloadOut != nullptr)
        {
            *payloadOut = "Authentication required";
        }
        return AUTHENTICATION_REQUIRED;
    }

    for (const auto &attr : method.security.requiredAttributes)
    {
        if (currentAttributes.find(attr) == currentAttributes.end())
        {
            if (payloadOut != nullptr)
            {
                *payloadOut = "Insufficient permissions";
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
        *payloadOut = "Internal error";
    }

    return INTERNAL_ERROR;
}

MethodsHandler::ErrorCodes MethodsHandler::invokeResource(const std::string &modeStr, const std::string &resourceName, const Parameters &inputParameters, const std::set<std::string> &currentAttributes, bool authenticated, Json::Value *payloadOut)
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
            *payloadOut = "Invalid method mode string";
        }
        return INVALID_METHOD_MODE;
    }

    return invokeResource(mode, resourceName, inputParameters, currentAttributes, authenticated, payloadOut);

}

