#include "httpv1_server.h"
#include <boost/algorithm/string/predicate.hpp>

using namespace Mantids30::Network::Protocol;
using namespace Mantids30::Network;
using namespace Mantids30;

using namespace std;

void HTTP::HTTPv1_Server::fillLogInformation(Json::Value &jWebLog)
{
    jWebLog["remoteHost"] = clientRequest.networkClientInfo.REMOTE_ADDR;
    jWebLog["timestamp"] = static_cast<Json::Int64>(time(nullptr));
    jWebLog["requestLine"] = clientRequest.requestLine.toString();
    jWebLog["referer"] = clientRequest.getHeaderOption("Referer");
    jWebLog["userAgent"] = clientRequest.getHeaderOption("User-Agent");
    jWebLog["responseStatus"] = static_cast<uint16_t>(serverResponse.status.getCode());

    size_t strsize;
    if ((strsize = serverResponse.content.getStreamSize()) != std::numeric_limits<size_t>::max())
    {
        jWebLog["bytesSent"] = strsize;
    }
}

bool HTTP::HTTPv1_Server::sendFullHTTPResponse()
{
    Json::Value jWebLog;
    fillLogInformation(jWebLog);
    log(jWebLog);

    if (!serverResponse.status.streamToUpstream())
    {
        // Bye... upstream failed (don't continue).
        m_currentSubParser = nullptr;
        return false;
    }

    // Stream Server HTTP Headers
    if (!sendHTTPHeadersResponse())
    {
        // Bye... upstream failed (don't continue).
        m_currentSubParser = nullptr;
        return false;
    }

    // The answer is the last thing... we move to the start or we drop the connection...
    if (connectionContinue)
    {
        // The connection must continue, because we have not reported or it's not marked as continue:
        m_currentSubParser = &clientRequest.requestLine;
        // Only the first request can upgrade a channel:
        prohibitConnectionUpgrade = true;
    }
    else
    {
        // Next parser is nullptr, means: no parsing handler for the next request.
        m_currentSubParser = nullptr;
    }

    // Stream content:
    bool streamedOK = serverResponse.content.streamToUpstream();

    // Destroy the binary content container here:
    serverResponse.content.setStreamableObject(nullptr);

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

    Json::Value jWebLog;
    fillLogInformation(jWebLog);
    log(jWebLog);

    // Not connection continue:
    if (!connectionContinue || clientRequest.getHeaderOption("Connection") == "close")
    {
        connectionContinue = false;
        serverResponse.headers.replace("Connection", "close");
    }

    // TODO: connection keep alive.
    // Size not specified...
    if ( (strsize = serverResponse.content.getStreamSize()) == std::numeric_limits<size_t>::max() )
    {
        serverResponse.headers.remove("Content-Length");
        /////////////////////
        if (serverResponse.content.getTransmissionMode() == HTTP::Content::TransmissionMode::CHUNKS)
        {
            // It's chunked...
            serverResponse.headers.replace("Transfer-Encoding", "Chunked");
        }
        else
        {
            // Not specified and not chunked? (close)
            connectionContinue = false;
            serverResponse.headers.replace("Connection", "close");
        }
    }
    else
    {
        serverResponse.headers.replace("Content-Length", std::to_string(strsize));
    }

    HTTP::Date currentDate;
    currentDate.setCurrentTime();

    if (serverResponse.includeDate)
    {
        serverResponse.headers.replace("Date", currentDate.toString());
    }

    if (serverResponse.immutableHeaders)
    {
        // No futher headers will be modified...
        bool r = serverResponse.headers.streamToUpstream();
        /*if (!r)
        {
            r = !r;
            r = !r;
        }*/
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
    {
        serverResponse.headers.replace("Cache-Control", cacheOptions);
    }

    if (!serverResponse.security.XFrameOptions.isNotActivated())
    {
        serverResponse.headers.replace("X-Frame-Options", serverResponse.security.XFrameOptions.toString());
    }

    // TODO: check if this is a secure connection.. (Over TLS?)
    if (serverResponse.security.HSTS.isActivated)
    {
        serverResponse.headers.replace("Strict-Transport-Security", serverResponse.security.HSTS.toString());
    }

    // Content Type...
    if (!serverResponse.contentType.empty())
    {
        serverResponse.headers.replace("Content-Type", serverResponse.contentType);
        if (serverResponse.security.disableNoSniffContentType)
        {
            serverResponse.headers.replace("X-Content-Type-Options", "nosniff");
        }
    }

    return serverResponse.headers.streamToUpstream();
}

bool HTTP::HTTPv1_Server::copyStreamToInternalResponseContent(const std::shared_ptr<Memory::Streams::StreamableObject> &source)
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
