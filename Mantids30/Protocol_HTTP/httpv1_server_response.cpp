#include "httpv1_server.h"
#include <boost/algorithm/string/predicate.hpp>

using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network;
using namespace Mantids30;

using namespace std;

void HTTP::HTTPv1_Server::fillLogInformation(Json::Value &jWebLog)
{
    jWebLog["remoteHost"] = clientRequest.networkClientInfo.REMOTE_ADDR;
    jWebLog["timestamp"] = (Json::UInt64)time(nullptr);
    jWebLog["requestLine"] = clientRequest.requestLine.toString();
    jWebLog["referer"] = clientRequest.getHeaderOption("Referer");
    jWebLog["userAgent"] = clientRequest.getHeaderOption("User-Agent");
    jWebLog["responseStatus"] = serverResponse.status.getCode();

    size_t strsize;
    if ((strsize = serverResponse.content.getStreamSize()) != std::numeric_limits<size_t>::max())
    {
        jWebLog["bytesSent"] = strsize;
    }
}

bool HTTP::HTTPv1_Server::sendFullHTTPResponse()
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "HTTP:Response");
#endif
    json jWebLog;
    fillLogInformation(jWebLog);
    log(jWebLog);

    // The answer is the last thing... we move to the start or we drop the connection...
    if (connectionContinue)
    {
        m_currentSubParser = (Memory::Streams::SubParser *) (&clientRequest.requestLine);
        prohibitConnectionUpgrade = true;
    }
    else
    {
        m_currentSubParser = nullptr;
        serverResponse.headers.replace("Connection", "close");
    }

    if (!serverResponse.status.streamToUpstream())
    {
        // Bye... upstream failed.
        m_currentSubParser = nullptr;
        return false;
    }

    // Stream Server HTTP Headers
    if (!sendHTTPHeadersResponse())
    {
        // Bye... upstream failed.
        m_currentSubParser = nullptr;
        return false;
    }

    // Stream content:
    bool streamedOK = serverResponse.content.streamToUpstream();

    // Destroy the binary content container here:
    serverResponse.content.setStreamableObj(nullptr);

    if (!streamedOK)
    {
        // Bye... upstream failed.
        m_currentSubParser = nullptr;
    }

    // Prepare the HTTP server for the next request...
    if (connectionContinue)
    {
        // Here we reset everything to the default values...
        reset();
    }


    return streamedOK;
}

bool HTTP::HTTPv1_Server::sendHTTPHeadersResponse()
{
    // Act as a server. Send data from here.
    size_t strsize;

    json jWebLog;
    fillLogInformation(jWebLog);
    log(jWebLog);

    // TODO: connection keep alive.
    if ((strsize = serverResponse.content.getStreamSize()) == std::numeric_limits<size_t>::max())
    {
        // Undefined size. (eg. dynamic stream)
        //serverResponse.headers.replace("Connection", "Close");
        serverResponse.headers.remove("Content-Length");
        /////////////////////
        if (serverResponse.content.getTransmitionMode() == HTTP::Content::TRANSMIT_MODE_CHUNKS)
            serverResponse.headers.replace("Transfer-Encoding", "Chunked");
    }
    else
    {
        std::string connectionType = serverResponse.headers.getOptionValueStringByName("Connection");
        if (boost::iequals(connectionType, "close"))
        {
            // On connection close, don't report the content size. (is there any reason for not reporting the size?)
        }
        else if (boost::iequals(connectionType, "upgrade"))
        {
            // conection type is defined as an upgrade, so no extra header / content length...
        }
        else
        {
            // Defined stream object size, reporting this to the client.
            serverResponse.headers.replace("Content-Length", std::to_string(strsize));
        }
    }

    HTTP::Date currentDate;
    currentDate.setCurrentTime();

    if (serverResponse.includeDate)
        serverResponse.headers.replace("Date", currentDate.toString());

    if (serverResponse.immutableHeaders)
    {
        // No futher headers will be modified...
        bool r = serverResponse.headers.streamToUpstream();
        if (!r)
        {
            r=!r;
            r=!r;
        }
        return r;
    }

    if (!serverResponse.sWWWAuthenticateRealm.empty())
    {
        serverResponse.headers.replace("WWW-Authenticate", "Basic realm=\"" + serverResponse.sWWWAuthenticateRealm + "\"");
    }

    // Establish the cookies
    serverResponse.headers.remove("Set-Cookie");
    serverResponse.cookies.putOnHeaders(&serverResponse.headers);

    // Security Options...
    serverResponse.headers.replace("X-XSS-Protection", serverResponse.security.XSSProtection.toString());

    std::string cacheOptions = serverResponse.cacheControl.toString();
    if (!cacheOptions.empty())
        serverResponse.headers.replace("Cache-Control", cacheOptions);

    if (!serverResponse.security.XFrameOpts.isNotActivated())
        serverResponse.headers.replace("X-Frame-Options", serverResponse.security.XFrameOpts.toString());

    // TODO: check if this is a secure connection.. (Over TLS?)
    if (serverResponse.security.HSTS.isActivated)
        serverResponse.headers.replace("Strict-Transport-Security", serverResponse.security.HSTS.toString());

    // Content Type...
    if (!serverResponse.contentType.empty())
    {
        serverResponse.headers.replace("Content-Type", serverResponse.contentType);
        if (serverResponse.security.disableNoSniffContentType)
            serverResponse.headers.replace("X-Content-Type-Options", "nosniff");
    }

    return serverResponse.headers.streamToUpstream();
}


bool HTTP::HTTPv1_Server::copyStreamToInternalResponseContent(std::shared_ptr<Memory::Streams::StreamableObject> source)
{
    if (!serverResponse.content.getStreamableObject())
    {
        return false;
    }
    // Stream in place:
    source->streamTo(serverResponse.content.getStreamableObject().get());
    return true;
}

std::shared_ptr<Memory::Streams::StreamableObject> HTTP::HTTPv1_Server::getResponseContentStreamableObject()
{
    return serverResponse.content.getStreamableObject();
}


