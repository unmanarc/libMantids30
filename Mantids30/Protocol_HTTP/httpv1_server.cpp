#include "httpv1_server.h"

#include <memory>
#include <vector>

#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/Memory/b_mmap.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace boost;
using namespace boost::algorithm;

using namespace std;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network;
using namespace Mantids30;

HTTP::HTTPv1_Server::HTTPv1_Server(std::shared_ptr<StreamableObject> sobject)
    : HTTPv1_Base(false, sobject)
{
    m_isInvalidHTTPRequest = false;

    // All request will have no-cache option activated.... (unless it's a real file and it's not overwritten)
    serverResponse.cacheControl.optionNoCache = true;
    serverResponse.cacheControl.optionNoStore = true;
    serverResponse.cacheControl.optionMustRevalidate = true;

    m_currentParser = (Memory::Streams::SubParser *) (&clientRequest.requestLine);

    loadDefaultMIMETypes();
}

bool HTTP::HTTPv1_Server::changeToNextParser()
{
    // Server Mode:
    if (m_currentParser == &clientRequest.requestLine)
        return changeToNextParserFromClientRequestLine();
    else if (m_currentParser == &clientRequest.headers)
        return changeToNextParserFromClientHeaders();
    else
        return changeToNextParserFromClientContentData();
}

bool HTTP::HTTPv1_Server::changeToNextParserFromClientHeaders()
{
    // This function is used when the client options arrives, so we need to parse it.

    auto parseHostHeader = [this]()
    {
        string hostVal = clientRequest.headers.getOptionValueStringByName("HOST");
        if (!hostVal.empty())
        {
            clientRequest.virtualPort = 80;
            vector<string> hostParts;
            split(hostParts, hostVal, is_any_of(":"), token_compress_on);
            if (hostParts.size() == 1)
            {
                clientRequest.virtualHost = hostParts[0];
            }
            else if (hostParts.size() > 1)
            {
                clientRequest.virtualHost = hostParts[0];
                char *endptr;
                unsigned long port = (uint16_t) strtoul(hostParts[1].c_str(), &endptr, 10);
                if (*endptr == '\0' && port <= 65535) // Check if conversion was successful and port is valid
                {
                    clientRequest.virtualPort = (uint16_t) port;
                }
                else
                {
                    // Invalid port number, set to default or handle error appropriately
                    clientRequest.virtualPort = 80;
                }
            }
        }
    };

    auto validateServerVersionRequirements = [this]()
    {
        if (clientRequest.requestLine.getHTTPVersion()->getMinor() >= 1)
        {
            if (clientRequest.virtualHost == "")
            {
                serverResponse.status.setCode(HTTP::Status::S_400_BAD_REQUEST);
                m_isInvalidHTTPRequest = true;
            }
        }
    };

    auto parseBasicAuthentication = [this]()
    {
        // PARSE CLIENT BASIC AUTHENTICATION:
        clientRequest.basicAuth.isEnabled = false;
        if (clientRequest.headers.exist("Authorization"))
        {
            vector<string> authParts;
            string f1 = clientRequest.headers.getOptionValueStringByName("Authorization");
            split(authParts, f1, boost::is_any_of(" "), token_compress_on);
            if (authParts.size() == 2)
            {
                if (authParts[0] == "Basic")
                {
                    auto bp = Helpers::Encoders::decodeFromBase64(authParts[1]);
                    std::string::size_type pos = bp.find(':', 0);
                    if (pos != std::string::npos)
                    {
                        clientRequest.basicAuth.isEnabled = true;
                        clientRequest.basicAuth.username = bp.substr(0, pos);
                        clientRequest.basicAuth.password = bp.substr(pos + 1, bp.size());
                    }
                }
            }
        }
    };

    auto parseUserAgentHeader = [this]()
    {
        // PARSE USER-AGENT
        if (clientRequest.headers.exist("User-Agent"))
            clientRequest.userAgent = clientRequest.headers.getOptionRawStringByName("User-Agent");
    };

    auto parseContentLengthAndType = [this](size_t &contentLength)
    {
        // PARSE CONTENT TYPE/LENGHT OPTIONS
        contentLength = clientRequest.headers.getOptionAsUINT64("Content-Length");
        string contentType = clientRequest.headers.getOptionValueStringByName("Content-Type");
        /////////////////////////////////////////////////////////////////////////////////////
        // Content-Length...
        if (contentLength)
        {
            // Content length defined.
            clientRequest.content.setTransmitionMode(HTTP::Content::TRANSMIT_MODE_CONTENT_LENGTH);
            if (!clientRequest.content.setContentLengthSize(contentLength))
            {
                // Error setting this content length size. (automatic answer)
                m_isInvalidHTTPRequest = true;
                serverResponse.status.setCode(HTTP::Status::S_413_PAYLOAD_TOO_LARGE);
            }
            /////////////////////////////////////////////////////////////////////////////////////
            // Content-Type... (only if length is designated)
            if (icontains(contentType, "multipart/form-data"))
            {
                clientRequest.content.setContainerType(HTTP::Content::CONTENT_TYPE_MIME);
                clientRequest.content.getMultiPartVars()->setMultiPartBoundary(clientRequest.headers.getOptionByName("Content-Type")->getSubVar("boundary"));
            }
            else if (icontains(contentType, "application/x-www-form-urlencoded"))
            {
                clientRequest.content.setContainerType(HTTP::Content::CONTENT_TYPE_URL);
            }
            else if (icontains(contentType, "application/json"))
            {
                clientRequest.content.setContainerType(HTTP::Content::CONTENT_TYPE_JSON);
            }
            else
                clientRequest.content.setContainerType(HTTP::Content::CONTENT_TYPE_BIN);
            /////////////////////////////////////////////////////////////////////////////////////
        }
    };

    parseHostHeader();
    validateServerVersionRequirements();
    parseBasicAuthentication();
    parseUserAgentHeader();

    if (m_isInvalidHTTPRequest)
        return sendHTTPResponse();
    else
    {
        size_t contentLength;
        parseContentLengthAndType(contentLength);

        if (m_isInvalidHTTPRequest)
            return sendHTTPResponse();
        else
        {
            // Process the client header options
            if (!procHTTPClientHeaders())
                m_currentParser = nullptr; // Don't continue with parsing (close the connection)
            else
            {
                if (contentLength)
                {
                    // The client set the content lenght, so we have to receive some data from the client (next parser is data content)
                    m_currentParser = &clientRequest.content;
                }
                else
                {
                    // If not, answer immediately.
                    return sendHTTPResponse();
                }
            }
        }
    }
    return true;
}

bool HTTP::HTTPv1_Server::changeToNextParserFromClientRequestLine()
{
    // Internal checks when URL request has received.
    prepareServerVersionOnURI();
    if (m_isInvalidHTTPRequest)
        return sendHTTPResponse();
    else
    {
        if (!procHTTPClientURI())
            m_currentParser = nullptr; // Don't continue with parsing.
        else
            m_currentParser = &clientRequest.headers;
    }
    return true;
}

bool HTTP::HTTPv1_Server::changeToNextParserFromClientContentData()
{
    // Don't continue with parsing. (TODO: Not supported yet)
    m_currentParser = nullptr;
    return sendHTTPResponse();
}

void HTTP::HTTPv1_Server::prepareServerVersionOnURI()
{
    serverResponse.status.getHTTPVersion()->setMajor(1);
    serverResponse.status.getHTTPVersion()->setMinor(0);

    if (clientRequest.requestLine.getHTTPVersion()->getMajor() != 1)
    {
        serverResponse.status.setCode(HTTP::Status::S_505_HTTP_VERSION_NOT_SUPPORTED);
        m_isInvalidHTTPRequest = true;
    }
    else
    {
        serverResponse.status.getHTTPVersion()->setMinor(clientRequest.requestLine.getHTTPVersion()->getMinor());
    }
}
