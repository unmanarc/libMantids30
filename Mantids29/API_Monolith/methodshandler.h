#pragma once

#include <map>

#include <Mantids29/Helpers/json.h>
#include <Mantids29/Threads/mutex_shared.h>
#include <string>
#include "methodsrequirements_map.h"

namespace Mantids29 { namespace API { namespace Monolith {
    
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
        json (*method)(void * obj, Mantids29::Auth::Session * session, const json & parameters);
        /**
         * @brief obj object to pass
         */
        void * obj;
    };

    /**
     * @brief MethodsHandler Constructor
     * @param appName Application Name
     */
    MethodsHandler(const std::string & appName);

    //////////////////////////////////////////////////

    bool addMethod(const std::string &methodName,
                   const std::set<std::string> &reqPermissions,
                   const MonolithAPIMethod &method,
                   const std::set<std::string> &reqActivities,
                   bool requireActiveSession = true);

    int invoke(Mantids29::Auth::Session *session, const std::string &methodName, const json &payload, json *payloadOut);

    eMethodValidationCodes validateMethodRequirements(Mantids29::Auth::Session *session, const std::string & methodName, json * reasons);

    Mantids29::Auth::MethodsRequirements_Map * methodsRequirements();

    bool doesMethodRequireActiveSession(const std::string & methodName);

    std::string getApplicationName() const;

private:
    //std::set<std::string> getApplicationPermissions(const std::set<std::string> & reqPermissions);

    /////////////////////////////////
    // Methods:

    // method name -> method.
    std::map<std::string,MonolithAPIMethod> m_methods;

    // method name -> bool (requireFullAuth).
    std::map<std::string,bool> m_methodRequireActiveSession;

    std::string m_applicationName;
    Mantids29::Auth::MethodsRequirements_Map m_methodsPermissions;

    // lock for methods manipulation...
    Threads::Sync::Mutex_Shared m_methodsMutex;
};

}}}

