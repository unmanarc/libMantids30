#pragma once

#include <Mantids30/Sessions/session.h>
#include <Mantids30/Memory/streamablestring.h>
#include "apiserverparameters.h"

#include <Mantids30/Memory/streamablejson.h>
#include <Mantids30/Protocol_HTTP/httpv1_server.h>
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Protocol_HTTP/api_return.h>
#include <memory>

namespace Mantids30 { namespace Network { namespace Servers { namespace Web {

class SessionInfo {
public:
    std::shared_ptr<Mantids30::Sessions::Session> authSession;
    std::string halfSessionId, sessionId;
    bool isImpersonation = false;
};

class APIClientHandler : public Protocols::HTTP::HTTPv1_Server
{
public:
    APIServerParameters * config;

    APIClientHandler(void *parent, std::shared_ptr<Memory::Streams::StreamableObject> sock);

protected:

    /**
     * @brief Processes the path parameters from a given request string.
     *
     * This function parses a request path string to extract the resource name and any
     * path parameters, storing them in a JSON object. It splits the request path using
     * '/' as a delimiter and treats the tokens as key-value pairs for the parameters.
     *
     * @param request The request path string to be processed (e.g., "/resource/key1/value1/key2/value2").
     * @param methodName A reference to a string where the extracted resource name will be stored.
     * @param pathParameters A reference to a `Json::Value` object where the extracted path parameters will be stored.
     *                       Each key in the path will be associated with its corresponding value in the JSON object.
     *                       If a key is not followed by a value, it will be assigned `Json::nullValue`.
     *
     * @details The function uses `boost::tokenizer` with a '/' separator to split the request into tokens.
     * - The first token is assumed to be the `methodName`.
     * - Subsequent tokens are treated as key-value pairs, where each key is followed by its value.
     * - If a key is not followed by a value, it is stored in `pathParameters` with `Json::nullValue`.
     *
     * Example:
     * - Input: `"/users/id/123/name/John"`
     * - `methodName` will be set to `"users"`.
     * - `pathParameters` will contain:
     *   ```
     *   {
     *     "id": "123",
     *     "name": "John"
     *   }
     *   ```
     */
    void processPathParameters(const std::string &request, std::string &methodName, Json::Value &pathParameters);

    /**
     * @brief procHTTPClientContent Process web client request
     * @return http response code.
     */
    Protocols::HTTP::Status::eRetCode procHTTPClientContent() override;
    /**
     * @brief sessionStart Retrieve/Start the session
     * @return S_200_OK for everything ok, any other value will return with that code immediatly.
     */
    virtual Protocols::HTTP::Status::eRetCode sessionStart() = 0;
    /**
     * @brief sessionCleanUp Clean up / release the session when finishing all the processing...
     * @return S_200_OK for good cleaning.
     */
    virtual void sessionCleanup() = 0;
    /**
     * @brief sessionFillVars Fill vars like csrf token, session max age and other related data to the session...
     * @param jVars vars to be filled
     */
    virtual void fillSessionExtraInfo( json & jVars ) {}
    /**
     * @brief fillSessionInfo Fill user data vars like username, domain, TLS Common Name...
     * @param jVars vars to be filled into a JSON structure
     */
    void fillSessionInfo ( json & jVars );
    /**
     * @brief doesSessionVariableExist check if a sesion variable exist.
     * @param varName variable name
     * @return return true if variable exist, else otherwise
     */
    virtual bool doesSessionVariableExist( const std::string & varName )= 0;
    /**
     * @brief getSessionVariableValue Get the session variable by name
     * @param varName variable name
     * @return return the session variable
     */
    virtual json getSessionVariableValue( const std::string & varName  )=0;

    /**
     * @brief Handles an API request and writes the response to the client.
     *
     * This method processes an API request by interpreting the provided parameters,
     * executing the appropriate logic for the specified API version, method, and mode,
     * and preparing a response that is written back to the client.
     *
     * @param apiReturn Pointer to an object where the result of the API request will be stored.
     * @param baseApiUrl The base URL for the API, used to construct resource paths or endpoints.
     * @param apiVersion The version of the API being requested.
     * @param methodMode The mode of the API method, such as GET, POST, PUT, DELETE, etc.
     * @param methodName The name of the API method to execute.
     * @param pathParameters A JSON object containing parameters parsed from the URL path.
     * @param postParameters A JSON object containing parameters sent in the POST body.
     *
     * @return Returns an appropriate API return code indicating success or the type of error encountered.
     */
    virtual void handleAPIRequest(API::APIReturn * apiReturn, const std::string & baseApiUrl,
                                  const uint32_t & apiVersion,
                                  const std::string &methodMode,
                                  const std::string &methodName,
                                  const Json::Value & pathParameters,
                                  const Json::Value & postParameters
                                  ) = 0;
    /**
     * @brief handleAuthFunctions Handle API Authentication Functions (login, logout, etc) and write the response to the client...
     * @return return code for api request
     */
    virtual Protocols::HTTP::Status::eRetCode handleAuthFunctions(const std::string & baseApiUrl,const std::string & authFunctionName) = 0;

    /**
     * @brief handleAuthFunctions Handle API Information (Version, endpoints, and so)...
     * @return return code for api request
     */
    virtual json handleAPIInfo(const std::string & baseApiUrl)
    {
        return Json::nullValue;
    }


    void log(Mantids30::Program::Logs::eLogLevels logSeverity,  const std::string &module, const uint32_t &outSize, const char *fmtLog,... );
    /**
     * @brief Verifies a JWT token and extracts user data if valid.
     *
     * This function validates a JWT token provided as a string and, if valid, populates
     * the user data structure with relevant information extracted from the token's claims.
     *
     * @param strToken The JWT token string to be verified.
     *
     * @return `true` if the token is valid and verification succeeds, `false` otherwise.
     *
     * @details
     * - The function utilizes `m_APIServerParameters->jwtValidator` to verify the provided
     *   JWT token (`strToken`). The verification result is stored in `result`.
     * - If the token is valid and the `m_JWTToken` object reflects a valid state:
     *   - The `userName` is set from the token's subject.
     *   - The `domainName` is extracted from the "domain" claim (using a macro `JSON_ASSTRING_D`
     *     to handle default values).
     *   - All claims and permissions from the token are stored in `m_userData`.
     *   - The `sessionId` and `halfSessionId` are set from the token's JWT ID.
     *   - The `loggedIn` flag is set to `true` if `userName` is not empty.
     *   - The `sessionActive` flag is set to `true` to indicate an active session.
     *
     * The function returns `true` if the token passes verification and the extracted data
     * is valid, otherwise `false`.
     */
    bool verifyToken( const std::string &strToken);
    virtual bool getIsInActiveSession() = 0;
    virtual std::set<std::string> getSessionPermissions() = 0;
    virtual std::set<std::string> getSessionRoles() = 0;
    // Function to check if the URL contains any invalid characters
    bool isURLSafe(const std::string& url);
    bool isRedirectPathSafeForAuth(const std::string& url);

    Protocols::HTTP::Status::eRetCode showBrowserMessage(const std::string & title, const std::string& message, Protocols::HTTP::Status::eRetCode returnCode);
    std::shared_ptr<Memory::Streams::StreamableString> createHTMLAlertMessage(const std::string & title, const std::string& message);

    DataFormat::JWT::Token m_JWTToken;
    // This session info is used for the logs and for filling htmli variables.
    SessionInfo m_currentSessionInfo;

protected:
    Protocols::HTTP::Status::eRetCode redirectUsingJS( const std::string & url );


private:
    Protocols::HTTP::Status::eRetCode handleRegularFileRequest();

    bool versionIsSupported(const std::string &versionStr, int minVersion);

    bool isSupportedUserAgent(const std::string &userAgent);

    friend class HTMLIEngine;
};

}}}}

