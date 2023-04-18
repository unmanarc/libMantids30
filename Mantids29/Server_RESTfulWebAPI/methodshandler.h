#ifndef METHODS_HANDLER_H
#define METHODS_HANDLER_H

#include <map>
#include <set>
#include <list>
#include <Mantids29/Helpers/json.h>
#include <Mantids29/Threads/mutex_shared.h>

namespace Mantids29 { namespace Network { namespace Servers { namespace RESTful {

struct Parameters {
    Json::Value postParameters;
    Json::Value getParameters;
    Json::Value pathParameters;
};

class MethodsHandler
{
public:
    enum MethodMode {
        GET=0,
        POST=1,
        PUT=2,
        DELETE=3
    };

    enum ErrorCodes {
        SUCCESS = 0,
        INVALID_METHOD_MODE = -1,
        RESOURCE_NOT_FOUND = -2,
        AUTHENTICATION_REQUIRED = -3,
        INSUFFICIENT_PERMISSIONS = -4,
        INTERNAL_ERROR = -5
    };

    struct RESTfulAPIMethod
    {
        struct Security {
            bool requireUserAuthentication = true;
            std::set<std::string> requiredAttributes;
        };

        Json::Value (*method)(void * obj, const RESTful::Parameters &inputParameters ) = nullptr;
        Security security;

        void * obj = nullptr;
    };
    MethodsHandler();

    bool addResource(const MethodMode & mode, const std::string & resourceName, const RESTfulAPIMethod & method);
    ErrorCodes invokeResource(const MethodMode & mode, const std::string & resourceName, const RESTful::Parameters &inputParameters, const std::set<std::string> &currentAttributes, bool authenticated, Json::Value *payloadOut);
    ErrorCodes invokeResource(const std::string & modeStr, const std::string & resourceName, const RESTful::Parameters &inputParameters, const std::set<std::string> &currentAttributes, bool authenticated, Json::Value *payloadOut);

private:
    std::map<std::string,RESTfulAPIMethod> m_methodsGET;
    std::map<std::string,RESTfulAPIMethod> m_methodsPOST;
    std::map<std::string,RESTfulAPIMethod> m_methodsPUT;
    std::map<std::string,RESTfulAPIMethod> m_methodsDELETE;

    Threads::Sync::Mutex_Shared m_methodsMutex;
};
}}}}

#endif // METHODS_HANDLER_H
