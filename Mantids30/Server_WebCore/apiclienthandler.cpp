#include "apiclienthandler.h"
#include "htmliengine.h"

#include <Mantids30/Helpers/crypto.h>
#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Memory/b_mmap.h>
#include <Mantids30/Memory/streamablestring.h>
#include <Mantids30/Protocol_HTTP/httpv1_base.h>
#include <Mantids30/Protocol_HTTP/rsp_status.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <inttypes.h>
#include <json/value.h>
#include <memory>
#include <regex>
#include <stdarg.h>
#include <string>
#include <vector>

#ifdef _WIN32
#include <stdlib.h>
#define realpath(N, R) _fullpath((R), (N), _MAX_PATH)
#endif

using namespace Mantids30::Program::Logs;
using namespace Mantids30::Network;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Memory;
using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30;
using namespace std;

APIClientHandler::APIClientHandler(void *parent, std::shared_ptr<StreamableObject> sock)
    : HTTPv1_Server(sock)
{}

void APIClientHandler::processPathParameters(const std::string &request, std::string &methodName, Json::Value &pathParameters)
{
    using Tokenizer = boost::tokenizer<boost::char_separator<char>>;
    boost::char_separator<char> sep("/");

    Tokenizer tokens(request, sep);
    Tokenizer::iterator tokenIterator = tokens.begin();

    // Extract the resource name (first token)
    if (tokenIterator != tokens.end())
    {
        methodName = *tokenIterator;
        ++tokenIterator;
    }

    // Extract key-value pairs and populate pathParameters
    while (tokenIterator != tokens.end())
    {
        std::string key = *tokenIterator;
        ++tokenIterator;

        if (tokenIterator != tokens.end())
        {
            std::string value = *tokenIterator;

            pathParameters[key] = value;

            ++tokenIterator;
        }
        else
        {
            // If a key is not followed by a value, assign it null.
            pathParameters[key] = Json::nullValue;
        }
    }
}

HTTP::Status::Codes APIClientHandler::procHTTPClientContent()
{
    HTTP::Status::Codes ret = HTTP::Status::S_404_NOT_FOUND;
    std::string requestURI = clientRequest.getURI();
    bool isAPIURI = false;

    if (!config->webServerName.empty())
    {
        setResponseServerName(config->webServerName);
    }

    // Check if the client's User-Agent header corresponds to a supported browser.
    // This function filters out insecure browsers that lack SameSite cookie support,
    // effectively blocking older, less secure clients from accessing the web server.
    if (!isSupportedUserAgent(clientRequest.userAgent))
    {
        return showBrowserMessage("Browser Upgrade Required",
                                  R"(
                                <h1>Browser Upgrade Required</h1>
                                <p>Your browser does not meet the security requirements to access this site.</p>
                                <p>To continue, please update your browser to the last version for enhanced security.</p>
                                )",
                                  HTTP::Status::S_426_UPGRADE_REQUIRED);
    }

    // Do forced redirections (before session's):
    if (config->redirections.find(requestURI) != config->redirections.end())
    {
        return serverResponse.setRedirectLocation(config->redirections[requestURI]);
    }

    HTTP::Status::Codes rtmp;
    if ((rtmp = sessionStart()) != HTTP::Status::S_200_OK)
    {
        return rtmp;
    }

    for (const auto &baseApiUrl : config->APIURLs)
    {
        if (boost::starts_with(requestURI, baseApiUrl + "/"))
        {
            std::string apiUrlWithoutBase = requestURI.substr(baseApiUrl.size());

            std::regex apiVersionResourcePattern("/v(\\d+)/(.+)"); // regex to match "/vN/resource" pattern
            std::smatch pathMatch;
            if (std::regex_match(apiUrlWithoutBase, pathMatch, apiVersionResourcePattern))
            {
                // It's an API request (with resource).
                API::APIReturn apiReturn;
                std::string methodName;
                json pathParameters;
                size_t apiVersion = std::stoul(pathMatch[1].str());
                string methodMode = clientRequest.requestLine.getRequestMethod().c_str();
                string requestOrigin = clientRequest.getOrigin();
                string requestXAPIKEY = clientRequest.headers.getOptionValueStringByName("x-api-key");
                std::string resourceAndPathParameters = pathMatch[2].str();

                apiReturn.getBodyDataStreamer()->setIsFormatted(config->useFormattedJSONOutput);
                serverResponse.setDataStreamer(apiReturn.getBodyDataStreamer());
                serverResponse.setContentType("application/json", true);

                processPathParameters(resourceAndPathParameters, methodName, pathParameters);

                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                /// API ORIGIN VALIDATIONS:
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                if (methodName == this->config->callbackAPIMethodName)
                {
                    // Check Login Origin... ALWAYS.
                    if (this->config->permittedLoginOrigins.find(requestOrigin) == this->config->permittedLoginOrigins.end())
                    {
                        log(LEVEL_SECURITY_ALERT, "restAPI", 2048, "Unauthorized Callback API Usage attempt from disallowed origin {origin=%s}", requestOrigin.c_str());
                        apiReturn.setError(HTTP::Status::S_401_UNAUTHORIZED, "invalid_security_context", "Disallowed Origin");
                        ret = HTTP::Status::S_401_UNAUTHORIZED;
                        isAPIURI = true;
                        break;
                    }
                }
                else
                {
                    // Check API Origin (if origin is provided)
                    if (!requestOrigin.empty())
                    {
                        if (!this->config->dynamicOriginValidator)
                        {
                            // Use the native method...
                            if (this->config->permittedAPIOrigins.find(requestOrigin) == this->config->permittedAPIOrigins.end())
                            {
                                log(LEVEL_SECURITY_ALERT, "restAPI", 2048, "Unauthorized API Usage attempt from disallowed origin {origin=%s}", requestOrigin.c_str());
                                apiReturn.setError(HTTP::Status::S_401_UNAUTHORIZED, "invalid_security_context", "Disallowed Origin");
                                ret = HTTP::Status::S_401_UNAUTHORIZED;
                                isAPIURI = true;
                                break;
                            }
                        }
                        else
                        {
                            if (!this->config->dynamicOriginValidator(requestOrigin, requestXAPIKEY))
                            {
                                log(LEVEL_SECURITY_ALERT, "restAPI", 2048, "Unauthorized API Usage attempt from disallowed origin via dynamic validator {origin=%s}", requestOrigin.c_str());
                                apiReturn.setError(HTTP::Status::S_401_UNAUTHORIZED, "invalid_security_context", "Disallowed Origin");
                                ret = HTTP::Status::S_401_UNAUTHORIZED;
                                isAPIURI = true;
                                break;
                            }
                        }
                    }
                }

                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                /// API CALL:
                ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                std::shared_ptr<Mantids30::Memory::Streams::StreamableJSON> jsonStreamable = clientRequest.getJSONStreamerContent();
                json postParameters = !jsonStreamable ? Json::nullValue : *(jsonStreamable->getValue());
                handleAPIRequest(&apiReturn, baseApiUrl, apiVersion, methodMode, methodName, pathParameters, postParameters);

                ret = apiReturn.getHTTPResponseCode();

                // Set cookies to the server (eg. refresher token cookie)...
                for (auto &i : apiReturn.cookiesMap)
                {
                    serverResponse.setCookie(i.first, i.second);
                }

                // Set headers to the server response (eg. CORS)...
                for (auto &i : apiReturn.httpExtraHeaders)
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
                ret = HTTP::Status::S_200_OK;
                serverResponse.setDataStreamer(jPayloadOutStr);
                serverResponse.setContentType("application/json", true);
                isAPIURI = true;
                break;
            }
            else
            {
                // Invalid Format...
                ret = HTTP::Status::S_404_NOT_FOUND;
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
                ret = route.second.handler(dynUrlWithoutBase, &clientRequest, &serverResponse, route.second.obj);

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

void APIClientHandler::fillSessionInfo(json &jVars)
{
    if (m_currentSessionInfo.authSession)
    {
        jVars["isImpersonation"] = m_currentSessionInfo.isImpersonation;
        jVars["impersonator"] = m_currentSessionInfo.authSession->getImpersonator();

        jVars["halfSessionID"] = m_currentSessionInfo.halfSessionId;
        jVars["user"] = m_currentSessionInfo.authSession->getUser();
        jVars["domain"] = m_currentSessionInfo.authSession->getDomain();
        jVars["loggedIn"] = true;
    }
    else
    {
        jVars["loggedIn"] = false;
    }

    jVars["userTLSCommonName"] = clientRequest.networkClientInfo.tlsCommonName;
    jVars["userIP"] = clientRequest.networkClientInfo.REMOTE_ADDR;
    jVars["userAgent"] = clientRequest.userAgent;
}

HTTP::Status::Codes APIClientHandler::handleRegularFileRequest()
{
    // WEB RESOURCE MODE:
    HTTP::Status::Codes ret = HTTP::Status::S_404_NOT_FOUND;
    sLocalRequestedFileInfo fileInfo;
    uint64_t uMaxAge = 0;

    // if there are no web resources path, return 404 without data.
    if (config->getDocumentRootPath().empty())
        return HTTP::Status::S_404_NOT_FOUND;

    if ( //staticContent ||
        (getLocalFilePathFromURI2(config->getDocumentRootPath(), &fileInfo, ".html") || getLocalFilePathFromURI2(config->getDocumentRootPath(), &fileInfo, "index.html")
         || getLocalFilePathFromURI2(config->getDocumentRootPath(), &fileInfo, ""))
        && !fileInfo.isDir)
    {
        // Evaluate...
        API::Web::ResourcesFilter::FilterEvaluationResult e;

        // if there is any resource filter, evaluate the sRealRelativePath with the action to be taken for that file
        // it will proccess this according to the authorization session
        if (config->resourceFilter)
        {
            e = config->resourceFilter->evaluateURI(fileInfo.sRealRelativePath, getSessionScopes(), getSessionRoles(), isSessionActive());
        }

        // If the element is accepted (during the filter)
        if (e.accept)
        {
            // and there is not redirect's, the resoponse code will be 200 (OK)
            if (e.redirectLocation.empty())
                ret = HTTP::Status::S_200_OK;
            else // otherwise you will need to redirect.
                ret = serverResponse.setRedirectLocation(e.redirectLocation);
        }
        else // If not, drop a 403 (forbidden)
            ret = HTTP::Status::S_403_FORBIDDEN;

        log(LEVEL_DEBUG, "fileServer", 2048, "R/ - LOCAL - %03" PRIu16 ": %s", static_cast<uint16_t>(ret), fileInfo.sRealFullPath.c_str());
    }
    else
    {
        // File not found at this point (404)
        log(LEVEL_WARN, "fileServer", 65535, "R/404: %s", clientRequest.getURI().c_str());
    }

    if (ret != HTTP::Status::S_200_OK)
    {
        // For NON-200 responses, will stream nothing....
        serverResponse.setDataStreamer(nullptr);
    }

    // If the URL is going to process the Interactive HTML Engine,
    // and the document content is text/html, then, process it as HTMLIEngine:
    if (config->useHTMLIEngine && (serverResponse.contentType == "text/html" || serverResponse.contentType == "application/javascript")) // The content type has changed during the map.
    {
        ret = HTMLIEngine::processResourceFile(this, fileInfo.sRealFullPath);
    }

    // And if the file is not found and there are redirections, set the redirection:
    if (ret == HTTP::Status::S_404_NOT_FOUND && !config->redirectPathOn404.empty())
    {
        ret = serverResponse.setRedirectLocation(config->redirectPathOn404);
    }

    // Log the response.
    log(ret == HTTP::Status::S_200_OK ? LEVEL_INFO : LEVEL_WARN, "fileServer", 2048, "R/%03" PRIu16 ": %s", static_cast<uint16_t>(ret),
        ret == HTTP::Status::S_200_OK ? fileInfo.sRealRelativePath.c_str() : clientRequest.getURI().c_str());

    return ret;
}

bool APIClientHandler::versionIsSupported(const std::string &versionStr, int minVersion)
{
    int version = strtol(versionStr.c_str(), 0, 10);

    // Failed to retrieve the version.
    if (version < 0)
        return false;

    return version >= minVersion;
}

bool APIClientHandler::isSupportedUserAgent(const std::string &userAgent)
{
    // Convert to lowercase for easier comparison
    std::string details = boost::algorithm::to_lower_copy(userAgent);

    // List of supported browsers and their minimum versions
    struct BrowserSupport
    {
        std::string name;
        int minVersion;
    };

    // Define supported browsers with minimum versions
    std::vector<BrowserSupport> supportedBrowsers = {

        // This is for chrome based browsers detection:
        {"chrome/", 51}, // Google Chrome -- Mozilla/5.0 (Windows NT 10.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/104.0.0.0 Safari/537.36
        //  {"chromium/", 51}, // Chromium -- Mozilla/5.0 (X11; Linux i686) AppleWebKit/535.1 (KHTML, like Gecko) Ubuntu/11.04 Chromium/14.0.825.0 Chrome/14.0.825.0 Safari/535.1
        //  {"edge/", 16}, // Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.102 Safari/537.36 Edge/18.19582
        // New Opera browsers uses chrome inside.
        //  {"opr/", 39}, // Mozilla/4.0 (compatible; MSIE 6.0; X11; Linux i686) Opera 7.23  [fi]
        //  {"opera mini/", 0}, // Opera Mini (all versions) -- Opera/9.80 (J2ME/MIDP; Opera Mini/4.2.13337/886; U; en) Presto/2.4.15
        //  {"opera mobi/", 80},      // Opera Mobile -- Opera/9.80 (Android 2.2.1; Linux; Opera Mobi/ADR-1107051709; U; pl) Presto/2.8.149 Version/11.10

        // This is for firefox detection:
        {"firefox/", 60}, // Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:101.0) Gecko/20100101 Firefox/101.0

        // This is for safari and other browsers detection (from iOS 12.3):
        {"applewebkit/", 606}, // Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9_3) AppleWebKit/537.75.14 (KHTML, like Gecko) Version/7.0.3 Safari/7046A194A
        //  {"safari/", 12}, // Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9_3) AppleWebKit/537.75.14 (KHTML, like Gecko) Version/7.0.3 Safari/7046A194A

        // This is for the old MSIE11:
        {"trident/", 7}, // IE 11 (alternate identifier) - // Mozilla/5.0 (compatible, MSIE 11, Windows NT 6.3; Trident/7.0;  rv:11.0) like Gecko
        //  {"msie ", 11},     // Mozilla/5.0 (compatible, MSIE 11, Windows NT 6.3; Trident/7.0;  rv:11.0) like Gecko

        // All CLI HTTP Clients (not HTML related can be used):
        {"curl/", 0},        // Curl (all versions)
        {"wget/", 0},        // Wget (all versions)
        {"httpie/", 0},      // HTTPie (all versions)
        {"libwww-perl/", 0}, // libwww-perl (all versions)
        {"libfetch/", 0},    // libfetch (all versions)
        {"axel/", 0},        // Axel (all versions)
        {"wget2/", 0},       // Wget2 (all versions)
        {"libmantids/", 0}   // Me :-)

    };

    // Check if the user agent matches any of the supported browsers
    for (const auto &browser : supportedBrowsers)
    {
        size_t pos = details.find(browser.name);
        if (pos != std::string::npos)
        {
            // Extract version number after the browser name
            size_t versionStart = pos + browser.name.size();
            size_t versionEnd = details.find(' ', versionStart); // Look for space or end of segment
            std::string versionStr = details.substr(versionStart, versionEnd - versionStart);

            // If all versions are supported, return true immediately
            if (browser.minVersion == 0)
            {
                return true;
            }

            // Otherwise, check if version meets the minimum requirement
            return versionIsSupported(versionStr, browser.minVersion);
        }
    }

    return false; // No supported browser matched
}

void APIClientHandler::log(eLogLevels logSeverity, const std::string &module, const uint32_t &outSize, const char *fmtLog, ...)
{
    va_list args;
    va_start(args, fmtLog);

    std::string user, domain;

    if (m_currentSessionInfo.authSession)
    {
        if (!m_currentSessionInfo.authSession->getImpersonator().empty())
        {
            user = m_currentSessionInfo.authSession->getUser() + "<-" + m_currentSessionInfo.authSession->getImpersonator();
        }
        else
        {
            user = m_currentSessionInfo.authSession->getUser();
        }

        domain = m_currentSessionInfo.authSession->getDomain();
    }

    if (config->rpcLog)
        config->rpcLog->logVA(logSeverity, clientRequest.networkClientInfo.REMOTE_ADDR, m_currentSessionInfo.halfSessionId, user, domain, module, outSize, fmtLog, args);

    va_end(args);
}

bool APIClientHandler::verifyToken(const std::string &strToken)
{
    // No token has been configured / for security, the token validation fails.
    if (this->config->jwtValidator == nullptr)
    {
        log(LEVEL_SECURITY_ALERT, "restAPI", 2048, "JWT token validation disabled: no JWT validator configured. Access denied for security reasons.");
        return false;
    }

    // Attempt to verify the provided token using the JWT validator.
    bool x = this->config->jwtValidator->verify(strToken, &m_JWTToken);

    if (!x)
        return false;

    // Check if the current running app matches the JWT spec.
    return JSON_ASSTRING_D(m_JWTToken.getClaim("app"), "") == config->appName;
}

bool APIClientHandler::isURLSafe(const std::string &url)
{
    for (char c : url)
    {
        // Allow only alphanumeric characters, '-', '_', '.', and '/'
        if (!(isalnum(c) || c == '-' || c == '_' || c == '.' || c == '/'))
        {
            return false; // Found an unsafe character
        }
    }
    return true; // URL is safe
}

bool APIClientHandler::isRedirectPathSafeForAuth(const std::string &url)
{
    for (const auto &apiurl : config->APIURLs)
    {
        if (boost::starts_with(url, apiurl + "/"))
        {
            return false;
        }
    }
    return true;
}

HTTP::Status::Codes APIClientHandler::redirectUsingJS(const std::string &url)
{
    if (url == "#retokenize")
        return Protocols::HTTP::Status::S_200_OK;

    std::shared_ptr<Memory::Streams::StreamableString> htmlOutput = std::make_shared<Memory::Streams::StreamableString>();
    htmlOutput->writeString("<script>window.location.href = atob('" + Helpers::Encoders::encodeToBase64(url) + "');</script>");
    serverResponse.setDataStreamer(htmlOutput);
    serverResponse.setContentType("text/html", true);

    return Protocols::HTTP::Status::S_200_OK;
}

HTTP::Status::Codes APIClientHandler::showBrowserMessage(const std::string &title, const std::string &message, Protocols::HTTP::Status::Codes returnCode)
{
    auto sHTMLPayloadOut = createHTMLAlertMessage(title, message);
    serverResponse.setDataStreamer(sHTMLPayloadOut);
    serverResponse.setContentType("text/html", true);
    return returnCode;
}

std::shared_ptr<Streams::StreamableString> APIClientHandler::createHTMLAlertMessage(const std::string &title, const std::string &message)
{
    std::shared_ptr<Memory::Streams::StreamableString> sPayloadOut = std::make_shared<Memory::Streams::StreamableString>();
    sPayloadOut->writeString(R"(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>)");
    sPayloadOut->writeString(title);
    sPayloadOut->writeString(R"(            </title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    color: #333;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    height: 100vh;
                    margin: 0;
                    background-color: #f4f4f4;
                }
                .container {
                    text-align: center;
                    max-width: 600px;
                    padding: 20px;
                    background-color: #fff;
                    border: 1px solid #ccc;
                    border-radius: 8px;
                    box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
                }
                h1 {
                    color: #d9534f;
                }
                p {
                    font-size: 1.1em;
                    margin-top: 10px;
                }
                a {
                    color: #0275d8;
                    text-decoration: none;
                }
                a:hover {
                    text-decoration: underline;
                }
            </style>
        </head>
        <body>
            <div class="container">
    )");
    sPayloadOut->writeString(message);
    sPayloadOut->writeString(R"(
            </div>
        </body>
        </html>
    )");
    return sPayloadOut;
}
