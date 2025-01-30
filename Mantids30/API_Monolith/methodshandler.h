#pragma once

#include <map>

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Threads/mutex_shared.h>
#include <memory>
#include <string>
#include "methodsrequirements_map.h"

namespace Mantids30 { namespace API { namespace Monolith {

class MethodsHandler
{
public:
    enum eMethodValidationCodes
    {
        VALIDATION_OK = 0x0,
        VALIDATION_METHODNOTFOUND = 0x1,
        VALIDATION_NOTAUTHORIZED = 0x2
    };

    enum eRetCodes {
        METHOD_RET_CODE_SUCCESS = 0,
        //METHOD_RET_CODE_INVALIDDOMAIN = -9993,
        METHOD_RET_CODE_UNAUTHENTICATED = -9994,
        METHOD_RET_CODE_INVALIDLOCALAUTH = -9995,
        METHOD_RET_CODE_TIMEDOUT = -9996,
        METHOD_RET_CODE_INVALIDAUTH = -9997,
        METHOD_RET_CODE_SERVERMEMORYFULL = -9998,
        METHOD_RET_CODE_METHODNOTFOUND = -9999
    };

    struct MonolithAPIMethod
    {
        /**
         * @brief Function pointer.
         */
        json (*method)(void * context,std::shared_ptr<Mantids30::Sessions::Session> session, const json & parameters);
        /**
         * @brief obj object to pass
         */
        void * context;
    };

    /**
     * @brief MethodsHandler Constructor
     * @param appName Application Name
     */
    MethodsHandler();

    //////////////////////////////////////////////////
    struct MethodDefinition
    {
        MonolithAPIMethod method;
        std::string methodName;
        std::set<std::string> reqPermissions;
        std::set<std::string> reqRoles;
        bool doUsageUpdateLastSessionActivity = true;
        bool isActiveSessionRequired = true;
    };
    bool addMethod(const MethodDefinition & methodDefinition);

    int invoke(std::shared_ptr<Sessions::Session> session, const std::string &methodName, const json &payload, json *payloadOut);

    eMethodValidationCodes validateMethodRequirements(std::shared_ptr<Mantids30::Sessions::Session> session, const std::string & methodName, json * reasons);

    MethodsRequirements_Map * methodsRequirements();

    bool doesMethodRequireActiveSession(const std::string & methodName);

   // std::string getApplicationName() const;

private:
    //std::set<std::string> getApplicationPermissions(const std::set<std::string> & reqPermissions);

    /////////////////////////////////
    // Methods:

    // method name -> method.
    std::map<std::string,MonolithAPIMethod> m_methods;

    // method name -> bool (RequireActiveSession).
    std::map<std::string,bool> m_methodRequireActiveSession;

    // method name -> bool (Update last activity on usage).
    std::map<std::string,bool> m_methodUpdateSessionLastActivityOnUsage;

    //std::string m_applicationName;
    MethodsRequirements_Map m_methodsPermissions;

    // lock for methods manipulation...
    Threads::Sync::Mutex_Shared m_methodsMutex;
};

}}}

