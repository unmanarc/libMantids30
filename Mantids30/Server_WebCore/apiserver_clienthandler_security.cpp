#include "apiserver_clienthandler.h"
#include <Mantids30/Helpers/json.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <cstdlib>
#include <string>
#include <vector>

using namespace Mantids30::Network;
using namespace Mantids30::Network::Servers::Web;
using namespace Mantids30;
using namespace Mantids30::Program;

using namespace std;

bool APIServer_ClientHandler::versionIsSupported(const std::string &versionStr, int minVersion)
{
    int version = strtol(versionStr.c_str(), nullptr, 10);

    // Failed to retrieve the version.
    if (version < 0)
    {
        return false;
    }

    return version >= minVersion;
}

bool APIServer_ClientHandler::isSupportedUserAgent(const std::string &userAgent)
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
        {"curl/", 0},           // Curl (all versions)
        {"wget/", 0},           // Wget (all versions)
        {"httpie/", 0},         // HTTPie (all versions)
        {"libwww-perl/", 0},    // libwww-perl (all versions)
        {"libfetch/", 0},       // libfetch (all versions)
        {"axel/", 0},           // Axel (all versions)
        {"wget2/", 0},          // Wget2 (all versions)
        {"PostmanRuntime/", 0}, // Postman (all versions)
        {"libmantids/", 0}      // Me :-)
    };

    // Check if the user agent matches any of the supported browsers
    for (const BrowserSupport &browser : supportedBrowsers)
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

bool APIServer_ClientHandler::verifyToken(const std::string &strToken)
{
    // No token has been configured / for security, the token validation fails.
    if (this->config->jwtValidator == nullptr)
    {
        log(Logs::LogLevel::SECURITY_ALERT, "restAPI", 2048, "JWT token validation disabled: no JWT validator configured. Access denied for security reasons.");
        return false;
    }

    // Attempt to verify the provided token using the JWT validator.
    bool x = this->config->jwtValidator->verify(strToken, &jwtToken);

    if (!x)
    {
        return false;
    }

    // Check if the current running app matches the JWT spec.
    if (Helpers::JSON::ASSTRING_D(jwtToken.getClaim("app"), "") != config->appName)
    {
        return false;
    }

    if (Helpers::JSON::ASSTRING_D(jwtToken.getClaim("type"), "") != "access")
    {
        return false;
    }

    return true;
}

bool APIServer_ClientHandler::isURLSafe(const std::string &url)
{
    for (char c : url)
    {
        // Allow only alphanumeric characters, '-', '_', '.', and '/'
        if (!isalnum(c) && c != '-' && c != '_' && c != '.' && c != '/')
        {
            return false; // Found an unsafe character
        }
    }
    return true; // URL is safe
}

bool APIServer_ClientHandler::isRedirectPathSafeForAuth(const std::string &url) const
{
    for (const std::string &apiurl : config->APIURLs)
    {
        if (boost::starts_with(url, apiurl + "/"))
        {
            return false;
        }
    }
    return true;
}