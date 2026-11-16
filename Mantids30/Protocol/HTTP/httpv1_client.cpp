#include "httpv1_client.h"

#include <Mantids30/Core/Helpers/encoders.h>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <memory>
#include <string>
#include <charconv>

using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Protocol;
using namespace Mantids30;

Mantids30::Protocol::HTTP::HTTPv1_Client::HTTPv1_Client(const std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> &sobject)
    : HTTPv1_Base(true, sobject)
{
    m_currentSubParser = &serverResponse.status;
    clientRequest.requestLine.getHTTPVersion()->setMajor(1);
    clientRequest.requestLine.getHTTPVersion()->setMinor(0);

    clientRequest.requestLine.setHTTPMethod("GET");
    clientRequest.userAgent = std::string("libMantids/") + std::to_string(HTTP_PRODUCT_VERSION_MAJOR) + std::string(".") + std::to_string(HTTP_PRODUCT_VERSION_MINOR);
}

bool Mantids30::Protocol::HTTP::HTTPv1_Client::initProtocol()
{
    if (!clientRequest.requestLine.streamToUpstream())
    {
        return false;
    }
    if (!streamClientHeaders())
    {
        return false;
    }
    if (!clientRequest.content.streamToUpstream())
    {
        return false;
    }

    // Succesfully initialized...
    return true;
}

bool Mantids30::Protocol::HTTP::HTTPv1_Client::changeToNextParser()
{
    if (m_currentSubParser == &serverResponse.status)
    {
        m_currentSubParser = &serverResponse.headers;
    }
    else if (m_currentSubParser == &serverResponse.headers)
    {
        const auto code = static_cast<uint16_t>(serverResponse.status.getCode());
        if (code >= 100 && code < 200 && code != 101)
        {
            serverResponse.headers.clear();
            m_currentSubParser = &serverResponse.status;
            return true;
        }
        // Process incoming server headers here:
        /////////////////////////////////////////////////////////////////////////

        // Parse Cache-Control
        serverResponse.cacheControl.fromString(serverResponse.headers.getOptionRawStringByName("Cache-Control"));

        // Parse ETag from response
        serverResponse.etag.fromString(serverResponse.headers.getOptionRawStringByName("ETag"));

        // Security:
        // Parse Xframeopts...
        serverResponse.security.XFrameOptions.fromString(serverResponse.headers.getOptionRawStringByName("X-Frame-Options"));
        // Parse XSS Protection.
        serverResponse.security.XSSProtection.fromString(serverResponse.headers.getOptionRawStringByName("X-XSS-Protection"));
        // Parse HSTS Configuration.
        serverResponse.security.HSTS.fromString(serverResponse.headers.getOptionRawStringByName("Strict-Transport-Security"));
        // Content No Sniff
        serverResponse.security.disableNoSniffContentType = iequals(serverResponse.headers.getOptionRawStringByName("X-Content-Type-Options"), "nosniff");

        // TODO: validate/check if using HSTS with SSL
        // TODO: get the preload list.
        std::string fullWWWAuthenticate = serverResponse.headers.getOptionRawStringByName("WWW-Authenticate");
        if (!fullWWWAuthenticate.empty())
        {
            // TODO: charset...
            boost::match_results<std::string::const_iterator> whatStaticText;
            boost::regex exStaticBasicREALM("^\\ *Basic\\ +Realm=\"(?<REALM>[^\"]*)\".*$", boost::regex::icase);

            boost::match_flag_type flags = boost::match_default;

            for (std::string::const_iterator start = fullWWWAuthenticate.begin(), end = fullWWWAuthenticate.end(); //
                 boost::regex_search(start, end, whatStaticText, exStaticBasicREALM, flags);                       // FIND REGEXP
                 start = whatStaticText[0].second)                                                                 // RESET AND RECHECK EVERYTHING
            {
                // TODO: url enconded
                serverResponse.sWWWAuthenticateRealm = std::string(whatStaticText[1].first, whatStaticText[1].second);
            }
        }

        // Parse content-type...
        m_serverContentType = serverResponse.headers.getOptionRawStringByName("Content-Type");
        serverResponse.contentType = m_serverContentType;

        // Parse server cookies...
        parseHeaders2ServerCookies();
        // Parse the transmition mode requested and act according it.
        m_currentSubParser = parseHeaders2TransmissionMode();
        if (m_responseFramingError)
        {
            return false;
        }
        if (!onResponseHeaders())
        {
            return false;
        }
        if (!m_currentSubParser && !serverResponse.content.getStreamableObject()->writeEOF())
        {
            m_parseResult = ParseResult::ERR_STREAM_WRITE;
            return false;
        }
    }
    else // END.
    {
        m_currentSubParser = nullptr;
    }
    return true;
}

void Mantids30::Protocol::HTTP::HTTPv1_Client::parseHeaders2ServerCookies()
{
    std::list<std::shared_ptr<Mantids30::Protocol::MIME::MIME_HeaderOption>> setCookies = serverResponse.headers.getOptionsByName("Set-Cookie");
    for (const std::shared_ptr<Mantids30::Protocol::MIME::MIME_HeaderOption> &serverCookie : setCookies)
    {
        serverResponse.cookies.parseCookie(serverCookie->getOrigValue());
    }
}

Mantids30::Core::Memory::Streams::SubParser *Mantids30::Protocol::HTTP::HTTPv1_Client::parseHeaders2TransmissionMode()
{
    serverResponse.content.setTransmissionMode(Mantids30::Protocol::HTTP::Content::TransmissionMode::CONNECTION_CLOSE);
    const auto code = static_cast<uint16_t>(serverResponse.status.getCode());
    if (clientRequest.requestLine.getHTTPMethod() == "HEAD" || code == 204 || code == 304 || (code >= 100 && code < 200))
    {
        serverResponse.content.setCurrentSize(0);
        return nullptr;
    }
    // Transfer-Encoding takes precedence over Content-Length.
    if (serverResponse.headers.exist("Transfer-Encoding"))
    {
        if (serverResponse.headers.getOptionsByName("Transfer-Encoding").size() != 1 ||
            !iequals(serverResponse.headers.getOptionRawStringByName("Transfer-Encoding"), "chunked"))
        {
            m_responseFramingError = true;
            return nullptr;
        }
        serverResponse.content.setTransmissionMode(Content::TransmissionMode::CHUNKS);
        return &serverResponse.content;
    }
    // Set Content Data Reception Mode.
    if (serverResponse.headers.exist("Content-Length"))
    {
        const auto values = serverResponse.headers.getOptionsByName("Content-Length");
        const std::string value = serverResponse.headers.getOptionRawStringByName("Content-Length");
        size_t len = 0;
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), len);
        if (values.size() != 1 || parsed.ec != std::errc() || parsed.ptr != value.data() + value.size())
        {
            m_responseFramingError = true;
            return nullptr;
        }
        serverResponse.content.setTransmissionMode(Mantids30::Protocol::HTTP::Content::TransmissionMode::CONTENT_LENGTH);

        // Error setting up that size or no data... (don't continue)
        if (!serverResponse.content.setCurrentSize(len))
        {
            m_responseFramingError = true;
            return nullptr;
        }
        if (!len)
        {
            return nullptr;
        }
    }

    return &serverResponse.content;
}

bool Mantids30::Protocol::HTTP::HTTPv1_Client::streamClientHeaders()
{
    // Act as a server. Send data from here.
    size_t strsize;

    if ((strsize = clientRequest.content.getStreamSize()) == std::numeric_limits<size_t>::max())
    {
        clientRequest.headers.remove("Content-Length");
        clientRequest.headers.replace("Transfer-Encoding", "chunked");
        clientRequest.content.setTransmissionMode(Content::TransmissionMode::CHUNKS);
    }
    else
    {
        clientRequest.headers.remove("Transfer-Encoding");
        clientRequest.content.setTransmissionMode(Content::TransmissionMode::CONTENT_LENGTH);
        //        clientRequest.headers.remove("Connection");
        clientRequest.headers.replace("Content-Length", std::to_string(strsize));

        if (strsize == 0 && clientRequest.requestLine.getHTTPMethod() != "POST" && clientRequest.requestLine.getHTTPMethod() != "PUT" &&
            clientRequest.requestLine.getHTTPMethod() != "PATCH")
        {
            clientRequest.headers.remove("Content-Length");
        }
    }

    if (!clientRequest.headers.exist("Content-Type"))
    {
        switch (clientRequest.content.getContainerType())
        {
        case Content::ContainerType::JSON:
            clientRequest.headers.replace("Content-Type", "application/json");
            break;
        case Content::ContainerType::URL:
            clientRequest.headers.replace("Content-Type", "application/x-www-form-urlencoded");
            break;
        case Content::ContainerType::MIME:
            if (clientRequest.content.isDefaultStreamableObj())
            {
                const auto mime = clientRequest.content.getMultiPartVars();
                clientRequest.headers.replace("Content-Type", mime->getMultiPartType() + "; boundary=" + mime->getMultiPartBoundary());
            }
            break;
        default:
            break;
        }
    }

    // Put client cookies:
    m_clientCookies.putOnHeaders(&clientRequest.headers);

    // Put If-None-Match header for conditional requests:
    if (!clientRequest.ifNoneMatch.isEmpty())
    {
        clientRequest.headers.replace("If-None-Match", clientRequest.ifNoneMatch.toString());
    }

    // Put basic authentication on headers:
    if (clientRequest.basicAuth.isEnabled)
    {
        clientRequest.headers.replace("Authorization", "Basic " + Mantids30::Core::Helpers::Encoders::encodeToBase64(clientRequest.basicAuth.username + ":" + clientRequest.basicAuth.password));
    }

    if (!clientRequest.headers.exist("User-Agent"))
    {
        clientRequest.headers.replace("User-Agent", clientRequest.userAgent);
    }

    // Put Virtual Host And Port (if exist)
    if (!clientRequest.virtualHost.empty() && !clientRequest.headers.exist("Host"))
    {
        clientRequest.headers.replace("Host", clientRequest.virtualHost + (clientRequest.virtualPort == 80 ? "" : ":" + std::to_string(clientRequest.virtualPort)));
    }
    // Stream it..
    return clientRequest.headers.streamToUpstream();
}

void Mantids30::Protocol::HTTP::HTTPv1_Client::endProtocol()
{
    if (!writeStatus.finished && m_currentSubParser == &serverResponse.content &&
        serverResponse.content.getTransmissionMode() == Content::TransmissionMode::CONNECTION_CLOSE)
    {
        writeEOF();
    }
}

void Mantids30::Protocol::HTTP::HTTPv1_Client::setBasicAuthentication(const std::string &user, const std::string &pass)
{
    clientRequest.basicAuth.setAuthentication(user, pass);
}

std::string Mantids30::Protocol::HTTP::HTTPv1_Client::getServerContentType() const
{
    return m_serverContentType;
}

void Mantids30::Protocol::HTTP::HTTPv1_Client::setClientRequest(const std::string &hostName, const std::string &uriPath)
{
    if (!hostName.empty())
    {
        clientRequest.requestLine.getHTTPVersion()->upgradeMinor(1);
    }
    clientRequest.requestLine.setRequestURI(uriPath);
    clientRequest.virtualHost = hostName;
}

Mantids30::Protocol::HTTP::HTTPv1_Client::PostMIMERequest Mantids30::Protocol::HTTP::HTTPv1_Client::prepareRequestAsPostMIME(const std::string &hostName, const std::string &uriPath)
{
    HTTPv1_Client::PostMIMERequest req;

    setClientRequest(hostName, uriPath);

    clientRequest.requestLine.setHTTPMethod("POST");
    clientRequest.content.setContainerType(Mantids30::Protocol::HTTP::Content::ContainerType::MIME);

    req.urlVars = std::dynamic_pointer_cast<Mantids30::Protocol::HTTP::URLVars>(clientRequest.requestLine.urlVars());
    req.postVars = clientRequest.content.getMultiPartVars();

    return req;
}

Mantids30::Protocol::HTTP::HTTPv1_Client::PostURLRequest Mantids30::Protocol::HTTP::HTTPv1_Client::prepareRequestAsPostURL(const std::string &hostName, const std::string &uriPath)
{
    HTTPv1_Client::PostURLRequest req;

    setClientRequest(hostName, uriPath);
    clientRequest.requestLine.setHTTPMethod("POST");
    clientRequest.content.setContainerType(Mantids30::Protocol::HTTP::Content::ContainerType::URL);
    req.urlVars = std::dynamic_pointer_cast<Mantids30::Protocol::HTTP::URLVars>(clientRequest.requestLine.urlVars());
    req.postVars = clientRequest.content.getUrlPostVars();

    return req;
}

std::string Mantids30::Protocol::HTTP::HTTPv1_Client::getResponseHeader(const std::string &headerName)
{
    return std::string(serverResponse.headers.getOptionValueStringByName(headerName));
}

void Mantids30::Protocol::HTTP::HTTPv1_Client::setReferer(const std::string &refererURL)
{
    clientRequest.requestLine.getHTTPVersion()->upgradeMinor(1);
    clientRequest.headers.replace("Referer", refererURL);
}

void Mantids30::Protocol::HTTP::HTTPv1_Client::addURLVar(const std::string &varName, const std::string &varValue)
{
    std::dynamic_pointer_cast<Mantids30::Protocol::HTTP::URLVars>(clientRequest.requestLine.urlVars())->addVar(varName, std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>(varValue));
}

void Mantids30::Protocol::HTTP::HTTPv1_Client::addCookie(const std::string &cookieName, const std::string &cookieVal)
{
    m_clientCookies.addCookieVal(cookieName, cookieVal);
}

void Mantids30::Protocol::HTTP::HTTPv1_Client::setIfNoneMatch(const std::string &etagValue)
{
    clientRequest.ifNoneMatch.fromString(etagValue);
}

void Mantids30::Protocol::HTTP::HTTPv1_Client::setIfNoneMatch(const Mantids30::Protocol::HTTP::Headers::ETag &etag)
{
    clientRequest.ifNoneMatch = etag;
}

Mantids30::Protocol::HTTP::Headers::ETag Mantids30::Protocol::HTTP::HTTPv1_Client::getResponseETag() const
{
    return serverResponse.etag;
}
