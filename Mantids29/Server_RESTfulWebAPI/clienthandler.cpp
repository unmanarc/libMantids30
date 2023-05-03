#include "clienthandler.h"
#include "Mantids29/API_RESTful/methodshandler.h"
#include "Mantids29/Program_Logs/loglevels.h"
#include <json/config.h>
#include <string>
#include <sstream>

#include <boost/tokenizer.hpp>
#include <iostream>
#include <string>
#include <json/json.h>

using namespace Mantids29::Program::Logs;
using namespace Mantids29::Network::Servers::RESTful;
using namespace Mantids29;
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
    std::string keyword, token;

    // Read the first two words from the input string
    iss >> keyword >> token;

    // TODO: Encrypted bearer support (you may don't want your client to know what is going on there...)
    if (keyword == "Bearer")
    {
        m_tokenVerified = m_jwtValidator->verify( token, &m_jwtToken );

        if (m_tokenVerified && m_jwtToken.isValid())
        {
            m_userData.userName = m_jwtToken.getSubject();
            m_userData.domainName =  JSON_ASSTRING_D(m_jwtToken.getClaim("domain"), "");
            m_userData.claims = m_jwtToken.getAllClaims();
            m_userData.halfSessionId = m_userData.sessionId = m_jwtToken.getJwtId();
            m_userData.attributes = m_jwtToken.getAllAttributes();

            if (!m_userData.userName.empty())
            {
                m_userData.loggedIn = true;
            }

            m_userData.sessionActive = true;
        }
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
    if ( m_tokenVerified )
    {
        jVars["maxAge"] = m_jwtToken.getExpirationTime() - time(nullptr);
    }
}

bool ClientHandler::doesSessionVariableExist(const std::string &varName)
{
    if ( m_tokenVerified )
    {
        return m_jwtToken.hasClaim(varName);
    }
    return false;
}

json ClientHandler::getSessionVariableValue(const std::string &varName)
{
    if ( m_tokenVerified )
    {
        return m_jwtToken.getClaim(varName);
    }
    return {};
}
Network::Protocols::HTTP::Status::eRetCode ClientHandler::handleAPIRequest(const std::string &baseApiUrl,const uint32_t & apiVersion, const std::string &resourceAndPathParameters)
{
    std::set<std::string> currentAttributes;
    bool authenticated =false;
    std::string resourceName;
    API::RESTful::InputParameters inputParameters;

    processPathParameters(resourceAndPathParameters,resourceName,inputParameters.pathParameters);
    inputParameters.jwtSigner = m_jwtSigner;
    inputParameters.jwtValidator = m_jwtValidator;
    inputParameters.clientRequest = &m_clientRequest;
    //inputParameters.REMOTE_ADDR = clientVars.REMOTE_ADDR;

    if (m_tokenVerified)
    {
        currentAttributes = m_jwtToken.getAllAttributes();
        inputParameters.jwtToken = &m_jwtToken;
    }

    API::RESTful::APIReturn apiReturn;
    apiReturn.body->setFormatted(m_config.useFormattedJSONOutput);

    m_serverResponse.setDataStreamer(apiReturn.body);
    m_serverResponse.setContentType("application/json",true);

    std::string methodMode = m_clientRequest.requestLine.getRequestMethod().c_str();

    if (m_methodsHandler.find(apiVersion) == m_methodsHandler.end())
    {
        log(eLogLevels::LEVEL_WARN, "restful", 2048, "API Version Not Found / Resource Not found {method=%s, mode=%s}", resourceName.c_str(), methodMode.c_str());
        *apiReturn.body = "Resource not found";
        return Protocols::HTTP::Status::S_404_NOT_FOUND;
    }

    json x;
    API::RESTful::MethodsHandler::ErrorCodes result = m_methodsHandler[apiVersion]->invokeResource( methodMode, resourceName, inputParameters, currentAttributes, m_userData.loggedIn, &apiReturn);

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


