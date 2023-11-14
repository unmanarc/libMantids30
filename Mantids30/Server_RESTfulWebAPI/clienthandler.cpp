#include "clienthandler.h"
#include <Mantids30/API_RESTful/methodshandler.h>
#include <Mantids30/Program_Logs/loglevels.h>
#include <json/config.h>
#include <string>
#include <sstream>

#include <boost/tokenizer.hpp>
#include <string>
#include <json/json.h>

using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network::Servers::RESTful;
using namespace Mantids30;
using namespace std;


ClientHandler::ClientHandler(void *parent, Memory::Streams::StreamableObject *sock) : Servers::Web::APIClientHandler(parent,sock)
{
    m_APIURLs.insert("/api");
}

ClientHandler::~ClientHandler()
{
}

Network::Protocols::HTTP::Status::eRetCode ClientHandler::sessionStart()
{
    // Check for the authorization bearer token...
    std::string authorization = m_clientRequest.getHeaderOption("Authorization");

    std::istringstream iss(authorization);
    std::string keyword, headerBearerToken;

    // Read the first two words from the input string
    iss >> keyword >> headerBearerToken;

    // Take the token from the cookie (if exist)...
    std::string cookieTokenValue = m_clientRequest.getCookie("AuthToken");

    if (!cookieTokenValue.empty())
    {
        m_JWTCookieTokenVerified = verifyToken(cookieTokenValue);
    }
    else if (keyword == "Bearer")
    {
        m_JWTHeaderTokenVerified = verifyToken(headerBearerToken);
    }

    return Protocols::HTTP::Status::S_200_OK;
}

Network::Protocols::HTTP::Status::eRetCode ClientHandler::sessionCleanup()
{
    // Cleaned up (nothing to do)
    return Protocols::HTTP::Status::S_200_OK;
}

void ClientHandler::sessionRenew()
{
    // Nothing to do... we are not the IAM
}

void ClientHandler::fillSessionVars(json &jVars)
{
    jVars["maxAge"] = 0;
    if ( m_JWTHeaderTokenVerified || m_JWTCookieTokenVerified )
    {
        jVars["maxAge"] = m_JWTToken.getExpirationTime() - time(nullptr);
    }
}

bool ClientHandler::doesSessionVariableExist(const std::string &varName)
{
    if ( m_JWTHeaderTokenVerified || m_JWTCookieTokenVerified )
    {
        return m_JWTToken.hasClaim(varName);
    }
    return false;
}

json ClientHandler::getSessionVariableValue(const std::string &varName)
{
    if ( m_JWTHeaderTokenVerified )
    {
        return m_JWTToken.getClaim(varName);
    }
    return {};
}

Network::Protocols::HTTP::Status::eRetCode ClientHandler::handleAPIRequest(const std::string &baseApiUrl,const uint32_t & apiVersion, const std::string &resourceAndPathParameters)
{
    API::RESTful::APIReturn apiReturn;
    apiReturn.body->setFormatted(m_config.useFormattedJSONOutput);
    m_serverResponse.setDataStreamer(apiReturn.body);
    m_serverResponse.setContentType("application/json",true);

    std::string methodMode = m_clientRequest.requestLine.getRequestMethod().c_str();
    std::set<std::string> currentPermissions;
    bool authenticated =false;
    std::string resourceName;
    API::RESTful::RequestParameters inputParameters;

    processPathParameters(resourceAndPathParameters,resourceName,inputParameters.pathParameters);
    inputParameters.jwtSigner = m_jwtSigner;
    inputParameters.jwtValidator = m_jwtValidator;
    inputParameters.clientRequest = &m_clientRequest;

    if (m_JWTHeaderTokenVerified || m_JWTCookieTokenVerified)
    {
        currentPermissions = m_JWTToken.getAllPermissions();
        inputParameters.jwtToken = &m_JWTToken;
    }

  /*  // Try to guess the CSRF here on POST's...
    if (m_JWTCookieTokenVerified)
    {
        if (m_clientRequest.headers.getOptionValueStringByName("AuthTokenHash") != m_clientRequest.getCookie("AuthTokenHash"))
        {
            log(eLogLevels::LEVEL_SECURITY_ALERT, "restful", 2048, "Attempted CSRF Attack detected {method=%s, mode=%s}", resourceName.c_str(), methodMode.c_str());
            apiReturn.setFullStatus(false, Network::Protocols::HTTP::Status::S_500_INTERNAL_SERVER_ERROR, "Internal Server Error");
            return apiReturn.code;
        }
    }*/

    if (m_methodsHandler.find(apiVersion) == m_methodsHandler.end())
    {
        log(eLogLevels::LEVEL_WARN, "restful", 2048, "API Version Not Found / Resource Not found {method=%s, mode=%s}", resourceName.c_str(), methodMode.c_str());
        apiReturn.setFullStatus(false, Network::Protocols::HTTP::Status::S_404_NOT_FOUND, "Resource Not Found");
        return apiReturn.code;
    }

    json x;
    API::RESTful::MethodsHandler::SecurityParameters securityParameters;
    securityParameters.haveJWTAuthCookie = m_JWTCookieTokenVerified;
    securityParameters.haveJWTAuthHeader = m_JWTHeaderTokenVerified;
    //inputParameters.cookies = m_clientRequest.getAllCookies();
    securityParameters.genCSRFToken = m_clientRequest.headers.getOptionValueStringByName("GenCSRFToken");

    API::RESTful::MethodsHandler::ErrorCodes result = m_methodsHandler[apiVersion]->invokeResource( methodMode, resourceName, inputParameters, currentPermissions, securityParameters, &apiReturn);

    switch (result)
    {
    case API::RESTful::MethodsHandler::SUCCESS:
        log(eLogLevels::LEVEL_DEBUG, "restful", 2048, "Method Executed {method=%s, mode=%s}", resourceName.c_str(), methodMode.c_str());
        break;
    case API::RESTful::MethodsHandler::INVALID_METHOD_MODE:
        log(eLogLevels::LEVEL_WARN, "restful", 2048, "Invalid Method Mode {method=%s, mode=%s}", resourceName.c_str(), methodMode.c_str());
        break;
    case API::RESTful::MethodsHandler::RESOURCE_NOT_FOUND:
        log(eLogLevels::LEVEL_WARN, "restful", 2048, "Method Not Found {method=%s, mode=%s}", resourceName.c_str(), methodMode.c_str());
        break;
    case API::RESTful::MethodsHandler::AUTHENTICATION_REQUIRED:
        log(eLogLevels::LEVEL_WARN, "restful", 2048, "Authentication Not Provided {method=%s, mode=%s}", resourceName.c_str(), methodMode.c_str());
        break;
    case API::RESTful::MethodsHandler::INSUFFICIENT_PERMISSIONS:
        log(eLogLevels::LEVEL_WARN, "restful", 2048, "Insufficient permissions {method=%s, mode=%s}", resourceName.c_str(), methodMode.c_str());
        break;
    case API::RESTful::MethodsHandler::INTERNAL_ERROR:
        log(eLogLevels::LEVEL_WARN, "restful", 2048, "Internal Error {method=%s, mode=%s}", resourceName.c_str(), methodMode.c_str());
        break;
    default:
        log(eLogLevels::LEVEL_ERR, "restful", 2048, "Unknown Error {method=%s, mode=%s}", resourceName.c_str(), methodMode.c_str());
        return Protocols::HTTP::Status::S_500_INTERNAL_SERVER_ERROR;
        break;
    }

    // Set cookies to the server (eg. refresher token cookie)...
    for (auto &i : apiReturn.cookiesMap)
    {
        m_serverResponse.setCookie(i.first, i.second);
    }

    return apiReturn.code;
}

void ClientHandler::processPathParameters(const std::string &request, std::string &resourceName, Json::Value &pathParameters)
{
    using Tokenizer = boost::tokenizer<boost::char_separator<char>>;
    boost::char_separator<char> sep("/");

    Tokenizer tokens(request, sep);
    Tokenizer::iterator tokenIterator = tokens.begin();

    if (tokenIterator != tokens.end())
    {
        resourceName = *tokenIterator;
        ++tokenIterator;
    }

    while (tokenIterator != tokens.end())
    {
        std::string key = *tokenIterator;
        ++tokenIterator;

        if (tokenIterator != tokens.end())
        {
            std::string value = *tokenIterator;
/*
            char* endPtr;
            long long intValue = std::strtoll(value.c_str(), &endPtr, 10);

            if (*endPtr == '\0')
            {
                pathParameters[key] = (Json::Int64) intValue;
            }
            else
            {*/
                pathParameters[key] = value;
            /*}*/

            ++tokenIterator;
        }
        else
        {
            pathParameters[key] = Json::nullValue;
        }
    }
}

bool ClientHandler::verifyToken( const std::string &strToken)
{
    // take the token from the header...
    bool result = m_jwtValidator->verify(strToken, &m_JWTToken);

    if (result && m_JWTToken.isValid())
    {
        m_userData.userName = m_JWTToken.getSubject();
        m_userData.domainName = JSON_ASSTRING_D(m_JWTToken.getClaim("domain"), "");
        m_userData.claims = m_JWTToken.getAllClaims();
        m_userData.halfSessionId = m_userData.sessionId = m_JWTToken.getJwtId();
        m_userData.permissions = m_JWTToken.getAllPermissions();

        if (!m_userData.userName.empty())
        {
            m_userData.loggedIn = true;
        }

        m_userData.sessionActive = true;
    }
    return result;
}
