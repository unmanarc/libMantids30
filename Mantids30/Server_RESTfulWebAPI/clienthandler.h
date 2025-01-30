#pragma once

#include <Mantids30/API_RESTful/methodshandler.h>
#include <Mantids30/Server_WebCore/apiclienthandler.h>
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <cstdint>

namespace Mantids30 { namespace Network { namespace Servers { namespace RESTful {

class ClientHandler : public Servers::Web::APIClientHandler
{
public:
    ClientHandler(void *parent, std::shared_ptr<Memory::Streams::StreamableObject> sock);
    ~ClientHandler() override;

protected:
    /**
     * @brief sessionStart Retrieve/Start the session
     * @return S_200_OK for everything ok, any other value will return with that code immediatly.
     */
    Protocols::HTTP::Status::eRetCode sessionStart() override;
    /**
     * @brief sessionCleanUp Clean up / release the session when finishing all the processing...
     */
    void sessionCleanup() override;
    /**
     * @brief fillSessionExtraInfo Fill vars like session max age and other related data to the session...
     * @param jVars vars to be filled
     */
    void fillSessionExtraInfo( json & jVars ) override;
    /**
     * @brief doesSessionVariableExist check if a sesion variable exist.
     * @param varName variable name
     * @return return true if variable exist, else otherwise
     */
    bool doesSessionVariableExist( const std::string & varName ) override;
    /**
     * @brief getSessionVariableValue Get the session variable by name
     * @param varName variable name
     * @return return the session variable
     */
    json getSessionVariableValue( const std::string & varName  ) override;
    /**
     * @brief handleAPIRequest Handle API Request and write the response to the client...
     * @return return code for api request
     */
    void handleAPIRequest(API::APIReturn *apiReturn, const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &methodMode, const std::string & methodName, const Json::Value &pathParameters, const Json::Value &postParameters) override;

    /**
     * @brief handleAuthFunctions Handle API Authentication Functions (login, logout, etc) and write the response to the client...
     * @return return code for api request
     */
    Protocols::HTTP::Status::eRetCode handleAuthFunctions(const std::string & baseApiUrl,const std::string & authFunctionName) override;


    bool getIsInActiveSession() override;
    std::set<std::string> getSessionPermissions() override;
    std::set<std::string> getSessionRoles() override;

private:

    void setPostLoginTokenCookie(const std::string &postLoginToken, const uint64_t &maxAge);
    Protocols::HTTP::Status::eRetCode handleAuthLoginFunction();

    void sessionLogout();

    // API Version -> MethodsHandler
    std::map<uint32_t,std::shared_ptr<API::RESTful::MethodsHandler>> m_methodsHandler;
    bool m_destroySession = false;
    bool m_JWTHeaderTokenVerified = false;
    bool m_JWTCookieTokenVerified = false;

    friend class Engine;
};

}}}}

