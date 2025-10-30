#include "httpv1_server.h"

using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network;
using namespace Mantids30;

using namespace std;


bool HTTP::HTTPv1_Server::sendHTTPResponse()
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "HTTP:Response");
#endif

    // Process client petition here.
    if (!m_isInvalidHTTPRequest)
    {
        serverResponse.status.setCode(procHTTPClientContent());
    }

    // Answer is the last... close the connection after it.
    m_currentParser = nullptr;

    if (!serverResponse.status.streamToUpstream())
    {
        return false;
    }

    if (!streamServerHeaders())
    {
        return false;
    }

    bool streamedOK = serverResponse.content.streamToUpstream();

    // Destroy the binary content container here:
    serverResponse.content.setStreamableObj(nullptr);

    return streamedOK;
}

bool HTTP::HTTPv1_Server::streamServerHeaders()
{
    // Act as a server. Send data from here.
    size_t strsize;

    if ((strsize = serverResponse.content.getStreamSize()) == std::numeric_limits<size_t>::max())
    {
        // TODO: connection keep alive.
        serverResponse.headers.replace("Connetion", "Close");
        serverResponse.headers.remove("Content-Length");
        /////////////////////
        if (serverResponse.content.getTransmitionMode() == HTTP::Content::TRANSMIT_MODE_CHUNKS)
            serverResponse.headers.replace("Transfer-Encoding", "Chunked");
    }
    else
    {
        serverResponse.headers.remove("Connetion");
        serverResponse.headers.replace("Content-Length", std::to_string(strsize));
    }

    HTTP::Date currentDate;
    currentDate.setCurrentTime();

    if (serverResponse.includeDate)
        serverResponse.headers.replace("Date", currentDate.toString());

    if (serverResponse.immutableHeaders)
    {
        // No futher headers will be modified...
        return serverResponse.headers.streamToUpstream();
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


bool HTTP::HTTPv1_Server::streamResponse(std::shared_ptr<Memory::Streams::StreamableObject> source)
{
    if (!serverResponse.content.getStreamableObj())
    {
        return false;
    }
    // Stream in place:
    source->streamTo(serverResponse.content.getStreamableObj().get());
    return true;
}

std::shared_ptr<Memory::Streams::StreamableObject> HTTP::HTTPv1_Server::getResponseDataStreamer()
{
    return serverResponse.content.getStreamableObj();
}


