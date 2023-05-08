#pragma once

#include <Mantids29/API_RESTful/methodshandler.h>
#include <Mantids29/Server_WebCore/apiclienthandler.h>
#include <Mantids29/DataFormat_JWT/jwt.h>
#include <cstdint>

namespace Mantids29 { namespace Network { namespace Servers { namespace RESTful {

class ClientHandler : public Servers::Web::APIClientHandler
{
public:
    ClientHandler(void *parent, Memory::Streams::StreamableObject *sock);
    ~ClientHandler() override;

    // JWT Validator and signer...
    std::shared_ptr<DataFormat::JWT> m_jwtValidator, m_jwtSigner;
    // API Version -> MethodsHandler
    std::map<uint32_t,std::shared_ptr<API::RESTful::MethodsHandler>> m_methodsHandler;

protected:
    /**
     * @brief sessionStart Retrieve/Start the session
     * @return S_200_OK for everything ok, any other value will return with that code immediatly.
     */
    Protocols::HTTP::Status::eRetCode sessionStart() override;
    /**
     * @brief sessionCleanUp Clean up / release the session when finishing all the processing...
     * @return S_200_OK for good cleaning.
     */
    Protocols::HTTP::Status::eRetCode sessionCleanup() override;
    /**
     * @brief sessionRenew Renew the session
     */
    void sessionRenew() override;
    /**
     * @brief sessionFillVars Fill vars like csrf token, session max age and other related data to the session...
     * @param jVars vars to be filled
     */
    void fillSessionVars( json & jVars ) override;
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
    Protocols::HTTP::Status::eRetCode handleAPIRequest(const std::string & baseApiUrl,const uint32_t & apiVersion, const std::string & resourceAndPathParameters) override;

    DataFormat::JWT::Token m_jwtToken;
    bool m_tokenVerified = false;
private:


    void processPathParameters(const std::string &request, std::string &resourceName, Json::Value &pathParameters);

};

}}}}

