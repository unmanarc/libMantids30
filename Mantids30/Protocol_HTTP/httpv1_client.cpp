#include "httpv1_client.h"

#include <Mantids30/Helpers/encoders.h>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <memory>
#include <string>

using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols::HTTP;
using namespace Mantids30;

HTTPv1_Client::HTTPv1_Client(std::shared_ptr<Memory::Streams::StreamableObject> sobject) : HTTPv1_Base(true,sobject)
{
    m_currentParser = (Memory::Streams::SubParser *)(&serverResponse.status);
    clientRequest.requestLine.getHTTPVersion()->setMajor(1);
    clientRequest.requestLine.getHTTPVersion()->setMinor(0);

    clientRequest.requestLine.setRequestMethod("GET");
    clientRequest.userAgent = std::string("libMantids/") + std::to_string(HTTP_PRODUCT_VERSION_MAJOR) + std::string(".") + std::to_string(HTTP_PRODUCT_VERSION_MINOR);
}

bool HTTPv1_Client::initProtocol()
{
    Memory::Streams::WriteStatus wrStat;

    if (!clientRequest.requestLine.streamToUpstream(wrStat))
        return false;
    if (!streamClientHeaders(wrStat))
        return false;
    if (!clientRequest.content.streamToUpstream(wrStat))
        return false;

    // Succesfully initialized...
    return true;
}

bool HTTPv1_Client::changeToNextParser()
{
    if (m_currentParser == &serverResponse.status)
        m_currentParser = &serverResponse.headers;
    else if (m_currentParser == &serverResponse.headers)
    {
        // Process incoming server headers here:
        /////////////////////////////////////////////////////////////////////////

        // Parse Cache-Control
        serverResponse.cacheControl.fromString(serverResponse.headers.getOptionRawStringByName("Cache-Control"));

        // Security:
        // Parse Xframeopts...
        serverResponse.security.XFrameOpts.fromString(serverResponse.headers.getOptionRawStringByName("X-Frame-Options"));
        // Parse XSS Protection.
        serverResponse.security.XSSProtection.fromString(serverResponse.headers.getOptionRawStringByName("X-XSS-Protection"));
        // Parse HSTS Configuration.
        serverResponse.security.HSTS.fromString(serverResponse.headers.getOptionRawStringByName("Strict-Transport-Security"));
        // Content No Sniff
        serverResponse.security.disableNoSniffContentType = iequals(serverResponse.headers.getOptionRawStringByName("X-Content-Type-Options"),"nosniff");

        // TODO: validate/check if using HSTS with SSL
        // TODO: get the preload list.
        auto fullWWWAuthenticate = serverResponse.headers.getOptionRawStringByName("WWW-Authenticate");
        if (!fullWWWAuthenticate.empty())
        {
            // TODO: charset...
            boost::match_results<std::string::const_iterator> whatStaticText;
            boost::regex exStaticBasicREALM("^\\ *Basic\\ +Realm=\"(?<REALM>[^\"]*)\".*$",boost::regex::icase);

            boost::match_flag_type flags = boost::match_default;

            for (std::string::const_iterator start = fullWWWAuthenticate.begin(), end =  fullWWWAuthenticate.end(); //
                 boost::regex_search(start, end, whatStaticText, exStaticBasicREALM, flags); // FIND REGEXP
                 start = whatStaticText[0].second) // RESET AND RECHECK EVERYTHING
            {
                // TODO: url enconded
                serverResponse.sWWWAuthenticateRealm = std::string(whatStaticText[1].first, whatStaticText[1].second);
            }
        }

        // Parse content-type...
        m_serverContentType = serverResponse.headers.getOptionRawStringByName("Content-Type");

        // Parse server cookies...
        parseHeaders2ServerCookies();
        // Parse the transmition mode requested and act according it.
        m_currentParser = parseHeaders2TransmitionMode();
    }
    else // END.
        m_currentParser = nullptr;
    return true;
}

void HTTPv1_Client::parseHeaders2ServerCookies()
{
    std::list<std::shared_ptr<MIME::MIME_HeaderOption>> setCookies = serverResponse.headers.getOptionsByName("");
    for (std::shared_ptr<MIME::MIME_HeaderOption> serverCookie : setCookies)
        serverResponse.cookies.parseCookie(serverCookie->getOrigValue());
}

Memory::Streams::SubParser * HTTPv1_Client::parseHeaders2TransmitionMode()
{
    serverResponse.content.setTransmitionMode(Common::Content::TRANSMIT_MODE_CONNECTION_CLOSE);
    // Set Content Data Reception Mode.
    if (serverResponse.headers.exist("Content-Length"))
    {
        uint64_t len = serverResponse.headers.getOptionAsUINT64("Content-Length");
        serverResponse.content.setTransmitionMode(Common::Content::TRANSMIT_MODE_CONTENT_LENGTH);

        // Error setting up that size or no data... (don't continue)
        if (!len || !serverResponse.content.setContentLenSize(len))
            return nullptr;
    }
    else if (icontains(serverResponse.headers.getOptionValueStringByName("Transfer-Encoding"),"CHUNKED"))
        serverResponse.content.setTransmitionMode(Common::Content::TRANSMIT_MODE_CHUNKS);

    return &serverResponse.content;
}

bool HTTPv1_Client::streamClientHeaders(Memory::Streams::WriteStatus &wrStat)
{
    // Act as a server. Send data from here.
    uint64_t strsize;

    // Can't use chunked mode on client.
    if ((strsize=clientRequest.content.getStreamSize()) == std::numeric_limits<uint64_t>::max())
        return false;
    else
    {
        clientRequest.headers.remove("Connetion");
        clientRequest.headers.replace("Content-Length", std::to_string(strsize));
    }

    // Put client cookies:
    m_clientCookies.putOnHeaders(&clientRequest.headers);

    // Put basic authentication on headers:
    if (clientRequest.basicAuth.isEnabled)
    {
        clientRequest.headers.replace("Authentication", "Basic " + Helpers::Encoders::encodeToBase64( clientRequest.basicAuth.username + ":" + clientRequest.basicAuth.password));
    }

    clientRequest.headers.replace("User-Agent", clientRequest.userAgent);

    // Put Virtual Host And Port (if exist)
    if (!clientRequest.virtualHost.empty())
        clientRequest.headers.replace("Host", clientRequest.virtualHost + (clientRequest.virtualPort==80?"":
                                                            ":"+std::to_string(clientRequest.virtualPort)) );
    // Stream it..
    return clientRequest.headers.streamToUpstream(wrStat);
}


std::string HTTPv1_Client::getServerContentType() const
{
    return m_serverContentType;
}

void HTTPv1_Client::setClientRequest(const std::string &hostName, const std::string &uriPath)
{
    if (!hostName.empty()) clientRequest.requestLine.getHTTPVersion()->upgradeMinor(1);
    clientRequest.requestLine.setRequestURI(uriPath);
    clientRequest.virtualHost = hostName;
}

HTTPv1_Client::PostMIMERequest HTTPv1_Client::prepareRequestAsPostMIME(const std::string &hostName, const std::string &uriPath)
{
    HTTPv1_Client::PostMIMERequest req;

    setClientRequest(hostName,uriPath);

    clientRequest.requestLine.setRequestMethod("POST");
    clientRequest.content.setContainerType(Common::Content::CONTENT_TYPE_MIME);

    req.urlVars = std::dynamic_pointer_cast<Common::URLVars>(clientRequest.requestLine.urlVars());
    req.postVars = clientRequest.content.getMultiPartVars();

    return req;
}

HTTPv1_Client::PostURLRequest HTTPv1_Client::prepareRequestAsPostURL(const std::string &hostName, const std::string &uriPath)
{
    HTTPv1_Client::PostURLRequest req;

    setClientRequest(hostName,uriPath);
    clientRequest.requestLine.setRequestMethod("POST");
    clientRequest.content.setContainerType(Common::Content::CONTENT_TYPE_URL);
    req.urlVars = std::dynamic_pointer_cast<Common::URLVars>(clientRequest.requestLine.urlVars());
    req.postVars = clientRequest.content.getUrlPostVars();

    return req;
}

std::string HTTPv1_Client::getResponseHeader(const std::string &headerName)
{
    return serverResponse.headers.getOptionValueStringByName(headerName);
}

void HTTPv1_Client::setReferer(const std::string &refererURL)
{
    clientRequest.requestLine.getHTTPVersion()->upgradeMinor(1);
    clientRequest.headers.replace("Referer", refererURL);
}

void HTTPv1_Client::addURLVar(const std::string &varName, const std::string &varValue)
{
    std::dynamic_pointer_cast<Common::URLVars>(clientRequest.requestLine.urlVars())->addVar(varName, std::make_shared<Memory::Containers::B_Chunks>(varValue));
}

void HTTPv1_Client::addCookie(const std::string &cookieName, const std::string &cookieVal)
{
    m_clientCookies.addCookieVal(cookieName, cookieVal);
}

