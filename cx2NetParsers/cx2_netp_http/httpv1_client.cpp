#include "httpv1_client.h"

#include <cx2_hlp_functions/encoders.h>
#include <boost/algorithm/string.hpp>

using namespace boost;
using namespace boost::algorithm;
using namespace CX2::Network::HTTP;
using namespace CX2;

HTTPv1_Client::HTTPv1_Client(Memory::Streams::Streamable *sobject) : HTTPv1_Base(true,sobject)
{
    currentParser = (Memory::Streams::Parsing::SubParser *)(&_serverCodeResponse);
    _clientRequestLine.getHTTPVersion()->setVersionMajor(1);
    _clientRequestLine.getHTTPVersion()->setVersionMinor(0);

    _clientRequestLine.setRequestMethod("GET");
}

bool HTTPv1_Client::initProtocol()
{
    Memory::Streams::Status wrStat;

    if (!_clientRequestLine.stream(wrStat))
        return false;
    if (!streamClientHeaders(wrStat))
        return false;
    if (!_clientContentData.stream(wrStat))
        return false;

    // Succesfully initialized...
    return true;
}

bool HTTPv1_Client::changeToNextParser()
{
    if (currentParser == &_serverCodeResponse)
        currentParser = &_serverHeaders;
    else if (currentParser == &_serverHeaders)
    {
        // Process headers here:
        /////////////////////////////////////////////////////////////////////////
        // Parse Xframeopts...
        secXFrameOpts.fromValue(_serverHeaders.getOptionRawStringByName("X-Frame-Options"));
        // Parse XSS Protection.
        secXSSProtection.fromValue(_serverHeaders.getOptionRawStringByName("X-XSS-Protection"));
        // Parse HSTS Configuration.
        secHSTS.fromValue(_serverHeaders.getOptionRawStringByName("Strict-Transport-Security"));
        // TODO: check if using HSTS
        // TODO: get the preload list.

        // Parse content-type...
        serverContentType = _serverHeaders.getOptionRawStringByName("Content-Type");
        securityNoSniffContentType = iequals(_serverHeaders.getOptionRawStringByName("X-Content-Type-Options"),"nosniff");
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
    std::list<MIME::MIME_HeaderOption *> setCookies = _serverHeaders.getOptionsByName("");
    for (MIME::MIME_HeaderOption * serverCookie : setCookies)
        serverCookies.parseCookie(serverCookie->getOrigValue());
}

Memory::Streams::Parsing::SubParser * HTTPv1_Client::parseHeaders2TransmitionMode()
{
    _serverContentData.setTransmitionMode(Common::TRANSMIT_MODE_CONNECTION_CLOSE);
    // Set Content Data Reception Mode.
    if (_serverHeaders.exist("Content-Length"))
    {
        uint64_t len = _serverHeaders.getOptionAsUINT64("Content-Length");
        _serverContentData.setTransmitionMode(Common::TRANSMIT_MODE_CONTENT_LENGTH);

        // Error setting up that size or no data... (don't continue)
        if (!len || !_serverContentData.setContentLenSize(len))
            return nullptr;
    }
    else if (icontains(_serverHeaders.getOptionValueStringByName("Transfer-Encoding"),"CHUNKED"))
        _serverContentData.setTransmitionMode(Common::TRANSMIT_MODE_CHUNKS);

    return &_serverContentData;
}

bool HTTPv1_Client::streamClientHeaders(Memory::Streams::Status &wrStat)
{
    // Act as a server. Send data from here.
    uint64_t strsize;

    // Can't use chunked mode on client.
    if ((strsize=_clientContentData.getStreamSize()) == std::numeric_limits<uint64_t>::max())
        return false;
    else
    {
        _clientHeaders.remove("Connetion");
        _clientHeaders.replace("Content-Length", std::to_string(strsize));
    }

    // Put client cookies:
    clientCookies.putOnHeaders(&_clientHeaders);

    // Stream it..
    return _clientHeaders.stream(wrStat);
}

Headers::Security::HSTS HTTPv1_Client::getSecHSTS() const
{
    return secHSTS;
}

Headers::Security::XSSProtection HTTPv1_Client::getSecXSSProtection() const
{
    return secXSSProtection;
}

Headers::Security::XFrameOpts HTTPv1_Client::getSecXFrameOpts() const
{
    return secXFrameOpts;
}

bool HTTPv1_Client::getSecurityNoSniffContentType() const
{
    return securityNoSniffContentType;
}

std::string HTTPv1_Client::getServerContentType() const
{
    return serverContentType;
}

Response::Cookies_ServerSide * HTTPv1_Client::getServerCookies()
{
    return &serverCookies;
}

void HTTPv1_Client::setClientRequest(const std::string &hostName, const std::string &uriPath)
{
    if (!hostName.empty()) _clientRequestLine.getHTTPVersion()->upgradeMinorVersion(1);
    _clientRequestLine.setRequestURI(uriPath);
    _clientHeaders.replace("Host", hostName);
}

void HTTPv1_Client::setDontTrackFlag(bool dnt)
{
    _clientRequestLine.getHTTPVersion()->upgradeMinorVersion(1);
    _clientHeaders.replace("DNT", dnt?"1":"0");
}

void HTTPv1_Client::setReferer(const std::string &refererURL)
{
    _clientRequestLine.getHTTPVersion()->upgradeMinorVersion(1);
    _clientHeaders.replace("Referer", refererURL);
}

void HTTPv1_Client::addCookie(const std::string &cookieName, const std::string &cookieVal)
{
    clientCookies.addCookieVal(cookieName, cookieVal);
}

void HTTPv1_Client::setClientUserAgent(const std::string &userAgent)
{
    _clientHeaders.replace("User-Agent", userAgent);
}

void HTTPv1_Client::setBasicAuthentication(const std::string &user, const std::string &pass)
{
    std::string authPlainText = user + ":" + pass;
    _clientHeaders.replace("Authentication", "Basic " + Helpers::Encoders::toBase64(authPlainText));
}
