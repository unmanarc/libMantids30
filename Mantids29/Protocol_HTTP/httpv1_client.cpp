#include "httpv1_client.h"

#include <Mantids29/Helpers/encoders.h>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <string>

using namespace boost;
using namespace boost::algorithm;
using namespace Mantids29::Network::Protocols::HTTP;
using namespace Mantids29;

HTTPv1_Client::HTTPv1_Client(Memory::Streams::StreamableObject *sobject) : HTTPv1_Base(true,sobject)
{
    currentParser = (Memory::Streams::SubParser *)(&m_serverResponse.status);
    m_clientRequest.requestLine.getHTTPVersion()->setMajor(1);
    m_clientRequest.requestLine.getHTTPVersion()->setMinor(0);

    m_clientRequest.requestLine.setRequestMethod("GET");
    m_clientRequest.userAgent = std::string("libMantids/") + std::to_string(HTTP_PRODUCT_VERSION_MAJOR) + std::string(".") + std::to_string(HTTP_PRODUCT_VERSION_MINOR);
}

bool HTTPv1_Client::initProtocol()
{
    Memory::Streams::StreamableObject::Status wrStat;

    if (!m_clientRequest.requestLine.stream(wrStat))
        return false;
    if (!streamClientHeaders(wrStat))
        return false;
    if (!m_clientRequest.content.stream(wrStat))
        return false;

    // Succesfully initialized...
    return true;
}

bool HTTPv1_Client::changeToNextParser()
{
    if (currentParser == &m_serverResponse.status)
        currentParser = &m_serverResponse.headers;
    else if (currentParser == &m_serverResponse.headers)
    {
        // Process incomming server headers here:
        /////////////////////////////////////////////////////////////////////////

        // Parse Cache-Control
        m_serverResponse.cacheControl.fromString(m_serverResponse.headers.getOptionRawStringByName("Cache-Control"));

        // Security:
        // Parse Xframeopts...
        m_serverResponse.security.XFrameOpts.fromString(m_serverResponse.headers.getOptionRawStringByName("X-Frame-Options"));
        // Parse XSS Protection.
        m_serverResponse.security.XSSProtection.fromString(m_serverResponse.headers.getOptionRawStringByName("X-XSS-Protection"));
        // Parse HSTS Configuration.
        m_serverResponse.security.HSTS.fromString(m_serverResponse.headers.getOptionRawStringByName("Strict-Transport-Security"));
        // Content No Sniff
        m_serverResponse.security.disableNoSniffContentType = iequals(m_serverResponse.headers.getOptionRawStringByName("X-Content-Type-Options"),"nosniff");

        // TODO: validate/check if using HSTS with SSL
        // TODO: get the preload list.
        auto fullWWWAuthenticate = m_serverResponse.headers.getOptionRawStringByName("WWW-Authenticate");
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
                m_serverResponse.sWWWAuthenticateRealm = std::string(whatStaticText[1].first, whatStaticText[1].second);
            }
        }

        // Parse content-type...
        m_serverContentType = m_serverResponse.headers.getOptionRawStringByName("Content-Type");

        // Parse server cookies...
        parseHeaders2ServerCookies();
        // Parse the transmition mode requested and act according it.
        currentParser = parseHeaders2TransmitionMode();
    }
    else // END.
        currentParser = nullptr;
    return true;
}

void HTTPv1_Client::parseHeaders2ServerCookies()
{
    std::list<MIME::MIME_HeaderOption *> setCookies = m_serverResponse.headers.getOptionsByName("");
    for (MIME::MIME_HeaderOption * serverCookie : setCookies)
        m_serverResponse.cookies.parseCookie(serverCookie->getOrigValue());
}

Memory::Streams::SubParser * HTTPv1_Client::parseHeaders2TransmitionMode()
{
    m_serverResponse.content.setTransmitionMode(Common::Content::TRANSMIT_MODE_CONNECTION_CLOSE);
    // Set Content Data Reception Mode.
    if (m_serverResponse.headers.exist("Content-Length"))
    {
        uint64_t len = m_serverResponse.headers.getOptionAsUINT64("Content-Length");
        m_serverResponse.content.setTransmitionMode(Common::Content::TRANSMIT_MODE_CONTENT_LENGTH);

        // Error setting up that size or no data... (don't continue)
        if (!len || !m_serverResponse.content.setContentLenSize(len))
            return nullptr;
    }
    else if (icontains(m_serverResponse.headers.getOptionValueStringByName("Transfer-Encoding"),"CHUNKED"))
        m_serverResponse.content.setTransmitionMode(Common::Content::TRANSMIT_MODE_CHUNKS);

    return &m_serverResponse.content;
}

bool HTTPv1_Client::streamClientHeaders(Memory::Streams::StreamableObject::Status &wrStat)
{
    // Act as a server. Send data from here.
    uint64_t strsize;

    // Can't use chunked mode on client.
    if ((strsize=m_clientRequest.content.getStreamSize()) == std::numeric_limits<uint64_t>::max())
        return false;
    else
    {
        m_clientRequest.headers.remove("Connetion");
        m_clientRequest.headers.replace("Content-Length", std::to_string(strsize));
    }

    // Put client cookies:
    m_clientCookies.putOnHeaders(&m_clientRequest.headers);

    // Put basic authentication on headers:
    if (m_clientRequest.basicAuth.isEnabled)
    {
        m_clientRequest.headers.replace("Authentication", "Basic " + Helpers::Encoders::encodeToBase64( m_clientRequest.basicAuth.username + ":" + m_clientRequest.basicAuth.password));
    }

    m_clientRequest.headers.replace("User-Agent", m_clientRequest.userAgent);

    // Put Virtual Host And Port (if exist)
    if (!m_clientRequest.virtualHost.empty())
        m_clientRequest.headers.replace("Host", m_clientRequest.virtualHost + (m_clientRequest.virtualPort==80?"":
                                                            ":"+std::to_string(m_clientRequest.virtualPort)) );
    // Stream it..
    return m_clientRequest.headers.stream(wrStat);
}


std::string HTTPv1_Client::getServerContentType() const
{
    return m_serverContentType;
}

void HTTPv1_Client::setClientRequest(const std::string &hostName, const std::string &uriPath)
{
    if (!hostName.empty()) m_clientRequest.requestLine.getHTTPVersion()->upgradeMinor(1);
    m_clientRequest.requestLine.setRequestURI(uriPath);
    m_clientRequest.virtualHost = hostName;
}

HTTPv1_Client::PostMIMERequest HTTPv1_Client::prepareRequestAsPostMIME(const std::string &hostName, const std::string &uriPath)
{
    HTTPv1_Client::PostMIMERequest req;

    setClientRequest(hostName,uriPath);

    m_clientRequest.requestLine.setRequestMethod("POST");
    m_clientRequest.content.setContainerType(Common::Content::CONTENT_TYPE_MIME);
    req.urlVars = (Common::URLVars *)m_clientRequest.requestLine.urlVars();
    req.postVars = m_clientRequest.content.getMultiPartVars();

    return req;
}

HTTPv1_Client::PostURLRequest HTTPv1_Client::prepareRequestAsPostURL(const std::string &hostName, const std::string &uriPath)
{
    HTTPv1_Client::PostURLRequest req;

    setClientRequest(hostName,uriPath);
    m_clientRequest.requestLine.setRequestMethod("POST");
    m_clientRequest.content.setContainerType(Common::Content::CONTENT_TYPE_URL);
    req.urlVars = (Common::URLVars *)m_clientRequest.requestLine.urlVars();
    req.postVars = m_clientRequest.content.getUrlPostVars();

    return req;
}

std::string HTTPv1_Client::getResponseHeader(const std::string &headerName)
{
    return m_serverResponse.headers.getOptionValueStringByName(headerName);
}

void HTTPv1_Client::setReferer(const std::string &refererURL)
{
    m_clientRequest.requestLine.getHTTPVersion()->upgradeMinor(1);
    m_clientRequest.headers.replace("Referer", refererURL);
}

void HTTPv1_Client::addURLVar(const std::string &varName, const std::string &varValue)
{
    ((Common::URLVars *)m_clientRequest.requestLine.urlVars())->addVar(varName, new Memory::Containers::B_Chunks(varValue));
}

void HTTPv1_Client::addCookie(const std::string &cookieName, const std::string &cookieVal)
{
    m_clientCookies.addCookieVal(cookieName, cookieVal);
}

