#include "httpv1_server.h"
#include <boost/algorithm/string/predicate.hpp>

using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network;
using namespace Mantids30;

using namespace std;


bool HTTP::HTTPv1_Server::sendFullHTTPResponse()
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "HTTP:Response");
#endif

    // Answer is the last... close the connection after it.
    m_currentSubParser = nullptr;

    if (!serverResponse.status.streamToUpstream())
    {
        return false;
    }

    // Stream Server HTTP Headers
    if (!sendHTTPHeadersResponse())
    {
        return false;
    }

    // Stream content:
    bool streamedOK = serverResponse.content.streamToUpstream();

    // Destroy the binary content container here:
    serverResponse.content.setStreamableObj(nullptr);

    return streamedOK;
}

bool HTTP::HTTPv1_Server::sendHTTPHeadersResponse()
{
    // Act as a server. Send data from here.
    size_t strsize;
    // TODO: connection keep alive.
    if ((strsize = serverResponse.content.getStreamSize()) == std::numeric_limits<size_t>::max())
    {
        // Undefined size. (eg. dynamic stream), the connection ends when closed.
        serverResponse.headers.replace("Connection", "Close");
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


