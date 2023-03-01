#ifndef SERVER_METHODS_H
#define SERVER_METHODS_H

#include <map>
#include <list>

#include <Mantids29/Helpers/json.h>

#include <Mantids29/Auth/domains.h>
#include <Mantids29/Threads/mutex_shared.h>

#include "methodsattributes_map.h"

namespace Mantids29 { namespace RPC {
    


class MethodsManager
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
        METHOD_RET_CODE_INVALIDDOMAIN = -9993,
        METHOD_RET_CODE_UNAUTHENTICATED = -9994,
        METHOD_RET_CODE_INVALIDLOCALAUTH = -9995,
        METHOD_RET_CODE_TIMEDOUT = -9996,
        METHOD_RET_CODE_INVALIDAUTH = -9997,
        METHOD_RET_CODE_SERVERMEMORYFULL = -9998,
        METHOD_RET_CODE_METHODNOTFOUND = -9999
    };

    struct sRPCMethod
    {
        /**
         * @brief Function pointer.
         */
        json (*rpcMethod)(void * obj, Mantids29::Authentication::Manager *, Mantids29::Authentication::Session * session, const json & parameters);
        /**
         * @brief obj object to pass
         */
        void * obj;
    };

    /**
     * @brief MethodsManager Constructor
     * @param appName Application Name
     */
    MethodsManager(const std::string & appName);

    //////////////////////////////////////////////////

    /**
     * @brief addRPCMethod Add New RPC Method
     * @param methodName Method Name (the name to be used in the rpc calls)
     * @param reqAttribs Required Attributes from the authenticator
     * @param rpcMethod RPC Method definition (struct) which include the function and one generic object pointer
     * @param requireFullAuth true if the method requires full user authentication, false for public methods.
     * @return true if the method was inserted.
     */
    bool addRPCMethod(const std::string & methodName, const std::set<std::string> & reqAttribs, const sRPCMethod & rpcMethod, bool requireFullAuth = true);
    /**
     * @brief runRPCMethod Run RPC Method
     * @param authDomain Authentication Domain Pool
     * @param domainName Domain to be used
     * @param auth Session Authentication
     * @param methodName Method to be executed
     * @param payload Payload for the method
     * @param payloadOut Response Payload (out)
     * @return 0 if succeed, -4 if method not found.
     */
    int runRPCMethod(Mantids29::Authentication::Domains *authDomain, const std::string &domainName, Mantids29::Authentication::Session *auth, const std::string & methodName, const json & payload, json *payloadOut);
    /**
     * @brief validateRPCMethodPerms Validate that RPC Method is authorized
     * @param auth Authentication Manager
     * @param session Session Authentication
     * @param methodName Method to be validated
     * @param extraTmpIndexes Extra Temporary Indexes
     * @param reasons Fancy Response
     * @return VALIDATION_OK/VALIDATION_METHODNOTFOUND/VALIDATION_NOTAUTHORIZED codes.
     */
    eMethodValidationCodes validateRPCMethodPerms(Authentication::Manager *auth, Mantids29::Authentication::Session *session, const std::string & methodName, const std::set<uint32_t> &extraTmpIndexes, json * reasons);
    /**
     * @brief getMethodsAttribs Use for method initialization only.
     * @return methods required attributes
     */
    Mantids29::Authentication::MethodsAttributes_Map * getMethodsAttribs();

    /**
     * @brief getMethodRequireFullAuth Get if a method requires full authentication
     * @param methodName Method Name
     * @return true if requires full session/authentication
     */
    bool getMethodRequireFullAuth(const std::string & methodName);
    /**
     * @brief getAppName Get Current Application Name
     * @return Application Name String
     */
    std::string getAppName() const;

private:
    std::set<Mantids29::Authentication::ApplicationAttribute> getAppAttribs(const std::set<std::string> & reqAttribs);

    json toValue(const std::set<Mantids29::Authentication::ApplicationAttribute> &t);

    // TODO: move to some helper
    json toValue(const std::set<std::string> &t);
    json toValue(const std::set<uint32_t> &t);

    // Methods:

    // method name -> method.
    std::map<std::string,sRPCMethod> methods;

    // method name -> bool (requireFullAuth).
    std::map<std::string,bool> methodRequireFullAuth;

    std::string appName;
    Mantids29::Authentication::MethodsAttributes_Map methodsAttribs;

    // lock for methods manipulation...
    Threads::Sync::Mutex_Shared smutexMethods;
};

}}

#endif // SERVER_METHODS_H
