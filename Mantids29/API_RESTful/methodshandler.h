#ifndef METHODS_HANDLER_H
#define METHODS_HANDLER_H

#include <map>
#include <set>
#include <list>
#include <Mantids29/Helpers/json.h>
#include <Mantids29/Threads/mutex_shared.h>
#include <Mantids29/DataFormat_JWT/jwt.h>

namespace Mantids29 { namespace API { namespace RESTful {

// Struct to hold HTTP request parameters
struct Parameters {
    Json::Value postParameters;     // Holds POST parameters
    Json::Value getParameters;      // Holds GET parameters
    Json::Value pathParameters;     // Holds parameters from the URL path
    DataFormat::JWT::Token emptyToken;
    DataFormat::JWT::Token * jwtToken = &emptyToken;    // Holds JWT token data, if present and validated the pointer will be changed.
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

    struct RESTfulAPIDefinition
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


    bool addResource(const MethodMode & mode, const std::string & resourceName,
                      Json::Value (*method)(void * obj, const RESTful::Parameters &inputParameters ),
                     void * obj,
                     bool requireUserAuthentication,
                     const std::set<std::string> requiredAttributes
                     );

    bool addResource(const MethodMode & mode, const std::string & resourceName, const RESTfulAPIDefinition & method);
    ErrorCodes invokeResource(const MethodMode & mode, const std::string & resourceName, const RESTful::Parameters &inputParameters, const std::set<std::string> &currentAttributes, bool authenticated, Json::Value *payloadOut);
    ErrorCodes invokeResource(const std::string & modeStr, const std::string & resourceName, const RESTful::Parameters &inputParameters, const std::set<std::string> &currentAttributes, bool authenticated, Json::Value *payloadOut);

private:
    std::map<std::string,RESTfulAPIDefinition> m_methodsGET;
    std::map<std::string,RESTfulAPIDefinition> m_methodsPOST;
    std::map<std::string,RESTfulAPIDefinition> m_methodsPUT;
    std::map<std::string,RESTfulAPIDefinition> m_methodsDELETE;

    Threads::Sync::Mutex_Shared m_methodsMutex;
};
}}}

#endif // METHODS_HANDLER_H
