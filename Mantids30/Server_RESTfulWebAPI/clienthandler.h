#pragma once

#include <Mantids30/API_RESTful/endpointshandler.h>
#include <Mantids30/Server_WebCore/apiclienthandler.h>
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <cstdint>

namespace Mantids30 { namespace Network { namespace Servers { namespace RESTful {

class ClientHandler : public Servers::Web::APIClientHandler
{
public:
    ClientHandler(void *parent, std::shared_ptr<Memory::Streams::StreamableObject> sock);

protected:
    /**
     * @brief sessionStart Retrieve/Start the session
     * @return S_200_OK for everything ok, any other value will return with that code immediatly.
     */
    Protocols::HTTP::Status::Codes sessionStart() override;
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
    API::APIReturn handleAPIRequest(const std::string &baseApiUrl, const uint32_t &apiVersion, const std::string &methodMode, const std::string & endpointName, const Json::Value &postParameters) override;

    /**
     * @brief handleAuthFunctions Handle API Authentication Functions (login, logout, etc) and write the response to the client...
     * @return return code for api request
     */
    Protocols::HTTP::Status::Codes handleAuthFunctions(const std::string & baseApiUrl,const std::string & authFunctionName) override;


    bool isSessionActive() override;
    std::set<std::string> getSessionScopes() override;
    std::set<std::string> getSessionRoles() override;

private:
    //void deliverSessionMaxAgeViaCookie(const uint64_t &maxAge);
//    void setAccessTokenAndLoggedInCookie(const std::string &token, const uint64_t &maxAge);
    //void setImpersonatorToken(const uint64_t &maxAge);
    //std::string getRedirectURL();

    //void setPostLoginTokenCookie(const std::string &postLoginToken, const uint64_t &maxAge);
    //Protocols::HTTP::Status::Codes handleAPIAuthCallbackFunction();

//    void sessionLogout();

    // API Version -> Endpoints
    std::map<uint32_t,std::shared_ptr<API::RESTful::Endpoints>> m_endpointsHandler;
    bool m_destroySession = false;
    bool m_JWTHeaderTokenVerified = false;
    bool m_JWTCookieTokenVerified = false;

    friend class Engine;
};

}}}}

