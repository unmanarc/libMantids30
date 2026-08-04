#include "apiserver_clienthandler.h"

#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Memory/streamable_string.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>


#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <json/value.h>
#include <memory>
#include <regex>
#include <string>

using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocol;
using namespace Mantids30::Memory;
using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30;
using namespace std;

APIServer_ClientHandler::APIServer_ClientHandler(void *parent, const std::shared_ptr<StreamableObject> &sock)
    : HTTPv1_Server(sock)
{}

HTTP::Status::Code APIServer_ClientHandler::onHTTPClientContentReceived()
{
    HTTP::Status::Code ret = HTTP::Status::Code::S_404_NOT_FOUND;
    std::string requestURI = clientRequest.getURI();
    bool isAPIURI = false;

    if (!config->webServerName.empty())
    {
        serverResponse.setServerName(config->webServerName);
    }

    // Check if the client's User-Agent header corresponds to a supported browser.
    // This function filters out insecure browsers that lack SameSite cookie support,
    // effectively blocking older, less secure clients from accessing the web server.
    if (!isSupportedUserAgent(clientRequest.userAgent))
    {
        HTTP::Status::Code retCode = showBrowserMessage("Browser Upgrade Required",
                                                        R"(
                                <h1>Browser Upgrade Required</h1>
                                <p>Your browser does not meet the security requirements to access this site.</p>
                                <p>To continue, please update your browser to the last version for enhanced security.</p>
                                )",
                                                        HTTP::Status::Code::S_426_UPGRADE_REQUIRED);
        return retCode;
    }

    if (config->dynamicInitialChecks)
    {
        HTTP::Status::Code retCode;
        if ((retCode = config->dynamicInitialChecks(&clientRequest, &serverResponse)) != HTTP::Status::Code::S_200_OK)
        {
            return retCode;
        }
    }

    // Do forced redirections (before session's):
    if (config->redirections.find(requestURI) != config->redirections.end())
    {
        HTTP::Status::Code retCode = serverResponse.setRedirectLocation(config->redirections[requestURI]);
        return retCode;
    }

    HTTP::Status::Code rtmp;
    if ((rtmp = sessionStart()) != HTTP::Status::Code::S_200_OK)
    {
        return rtmp;
    }

    // TODO: implement identity.

    logUsername.clear();
    if (currentSessionInfo.authSession)
    {
        logUsername = currentSessionInfo.authSession->getUser();
    }

    for (const std::string &baseApiUrl : config->APIURLs)
    {
        if (boost::starts_with(requestURI, baseApiUrl + "/"))
        {
            std::string apiUrlWithoutBase = requestURI.substr(baseApiUrl.size());

            std::regex apiVersionResourcePattern("/v(\\d+)/(.+)"); // regex to match "/vN/resource" pattern
            std::smatch pathMatch;
            if (std::regex_match(apiUrlWithoutBase, pathMatch, apiVersionResourcePattern))
            {
                // It's an API request (with resource).
                size_t apiVersion = std::stoul(pathMatch[1].str());
                string httpMethodMode = clientRequest.requestLine.getHTTPMethod();
                string requestOrigin = clientRequest.getOrigin();
                string requestXAPIKEY = clientRequest.headers.getOptionValueStringByName("x-api-key");
                std::string endpointName = pathMatch[2].str();

                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                /// API ORIGIN VALIDATIONS:
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                if (endpointName == this->config->loginCallbackAPIEndpointName)
                {
                    // This is for the callback endpoint
                    APIServerConfig::DynamicOriginValidatorFunction defaultCallbackOriginValidator =
                        [](const std::string &requestOrigin, const std::string &apikey, const std::set<std::string> &permittedCallbackOrigins) -> bool
                    { return permittedCallbackOrigins.count(requestOrigin); };

                    APIServerConfig::DynamicOriginValidatorFunction originValidator = this->config->dynamicLoginCallbackOriginValidator ? this->config->dynamicLoginCallbackOriginValidator
                                                                                                                                        : defaultCallbackOriginValidator;

                    // Check Login Origin... ALWAYS.
                    if (!originValidator(requestOrigin, requestXAPIKEY, this->config->permittedLoginOrigins))
                    {
                        API::APIReturn apiReturn;
                        apiReturn.getBodyDataStreamer()->setIsFormatted(config->useFormattedJSONOutput);
                        serverResponse.setContentDataStreamer(apiReturn.getBodyDataStreamer());
                        serverResponse.setContentType("application/json", true);

                        log(LogLevel::SECURITY_ALERT, "restAPI", 2048, "Unauthorized Callback API Usage attempt from disallowed origin {origin=%s}", requestOrigin.c_str());
                        apiReturn.setError(HTTP::Status::Code::S_401_UNAUTHORIZED, "invalid_security_context", "Disallowed Origin");
                        ret = apiReturn.getHTTPResponseCode();
                        isAPIURI = true;
                        break;
                    }
                }
                else
                {
                    // This is for the rest of the endpoints...
                    APIServerConfig::DynamicOriginValidatorFunction defaultOriginValidator =
                        [](const std::string &requestOrigin, const std::string &apikey, const std::set<std::string> &permittedAPIOrigins) -> bool { return permittedAPIOrigins.count(requestOrigin); };

                    APIServerConfig::DynamicOriginValidatorFunction originValidator = this->config->dynamicOriginValidator ? this->config->dynamicOriginValidator : defaultOriginValidator;

                    // Check API Origin (if origin is provided)
                    if (!requestOrigin.empty())
                    {
                        if (!originValidator(requestOrigin, requestXAPIKEY, this->config->permittedAPIOrigins))
                        {
                            API::APIReturn apiReturn;
                            apiReturn.getBodyDataStreamer()->setIsFormatted(config->useFormattedJSONOutput);
                            serverResponse.setContentDataStreamer(apiReturn.getBodyDataStreamer());
                            serverResponse.setContentType("application/json", true);

                            log(LogLevel::SECURITY_ALERT,
                                "restAPI",
                                2048,
                                "Unauthorized API Usage attempt from disallowed origin via %s validator {origin=%s}",
                                originValidator == defaultOriginValidator ? "default" : "dynamic",
                                requestOrigin.c_str());

                            apiReturn.setError(HTTP::Status::Code::S_401_UNAUTHORIZED, "invalid_security_context", "Disallowed Origin");
                            ret = apiReturn.getHTTPResponseCode();
                            isAPIURI = true;
                            break;
                        }
                    }
                }

                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                /// API CALL:
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                API::APIReturn apiReturn;

                std::shared_ptr<Mantids30::Memory::Streams::StreamableJSON> jsonStreamable = clientRequest.getJSONStreamerContent();
                Json::Value postParameters = !jsonStreamable ? Json::nullValue : *(jsonStreamable->getValue());

                if (httpMethodMode == "OPTIONS")
                {
                    apiReturn = handleOptionsRequest(baseApiUrl, apiVersion, endpointName);
                }
                else
                {
                    apiReturn = handleAPIRequest(baseApiUrl, apiVersion, httpMethodMode, endpointName, postParameters);
                    apiReturn.getBodyDataStreamer()->setIsFormatted(config->useFormattedJSONOutput);
                    serverResponse.setContentDataStreamer(apiReturn.getBodyDataStreamer());
                    serverResponse.setContentType("application/json", true);
                }

                ret = apiReturn.getHTTPResponseCode();

                // Set cookies to the server (eg. refresher token cookie)...
                for (const auto &i : apiReturn.cookiesMap)
                {
                    serverResponse.setCookie(i.first, i.second);
                }

                // Set headers to the server response (eg. CORS)...
                for (const auto &i : apiReturn.httpExtraHeaders)
                {
                    serverResponse.headers.add(i.first, i.second);
                }

                if (!apiReturn.redirectURL.empty())
                {
                    // Set as HTML.
                    ret = redirectUsingJS(apiReturn.redirectURL);
                }
                isAPIURI = true;
                break;
            }
            else if (boost::starts_with(apiUrlWithoutBase, "/auth/"))
            {
                std::string authFunction = apiUrlWithoutBase.substr(6);
                ret = handleAuthFunctions(baseApiUrl, authFunction);
                isAPIURI = true;
                break;
            }
            else if (boost::starts_with(apiUrlWithoutBase, "/info"))
            {
                std::shared_ptr<Memory::Streams::StreamableJSON> jPayloadOutStr = std::make_shared<Memory::Streams::StreamableJSON>();
                jPayloadOutStr->setIsFormatted(this->config->useFormattedJSONOutput);
                jPayloadOutStr->setValue(handleAPIInfo(baseApiUrl));
                ret = HTTP::Status::Code::S_200_OK;
                serverResponse.setContentDataStreamer(jPayloadOutStr);
                serverResponse.setContentType("application/json", true);
                isAPIURI = true;
                break;
            }
            else
            {
                // Invalid Format...
                ret = HTTP::Status::Code::S_404_NOT_FOUND;
                break;
            }
        }
    }

    // If the request is not identified as an API URI, proceed to handle it as a file or delegate to an appropriate handler.
    if (!isAPIURI)
    {
        // Flag to check if a matching dynamic content handler was found.
        bool isDynamicContent = false;

        // Iterate through all registered dynamic request handlers in the map.
        for (const auto &route : config->dynamicRequestHandlersByRoute)
        {
            // Check if the request URI starts with the route's path, indicating a match.
            if (boost::starts_with(requestURI, route.first + "/"))
            {
                // Extract the portion of the request URI that follows the matching route path.
                std::string dynUrlWithoutBase = requestURI.substr(route.first.size());

                // Invoke the corresponding handler function with the extracted path and request/response objects.
                ret = route.second.handler(dynUrlWithoutBase, &clientRequest, &serverResponse, route.second.obj, &currentSessionInfo);

                // Set the flag to true, indicating the request was processed as dynamic content.
                isDynamicContent = true;

                // Exit the loop as the matching handler has been found and executed.
                break;
            }
        }

        // If no matching dynamic handler was found, process the request as a regular file request.
        if (!isDynamicContent)
        {
            ret = handleRegularFileRequest();
        }
    }

    sessionCleanup();

    return ret;
}