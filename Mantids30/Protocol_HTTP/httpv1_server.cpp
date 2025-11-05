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


 /**
 * @brief Constructor for HTTPv1_Server.
 *
 * Initializes the HTTP/1.x server-side parser, sets default cache control headers,
 * and configures the initial parser state to process the request line.
 *
 * @param connectionStream Shared pointer to the streamable object used for communication.
 */
HTTP::HTTPv1_Server::HTTPv1_Server(std::shared_ptr<StreamableObject> connectionStream)
    : HTTPv1_Base(false, connectionStream)
{
    // Default to a conservative cache policy for dynamic responses.
    serverResponse.cacheControl.optionNoCache = true;
    serverResponse.cacheControl.optionNoStore = true;
    serverResponse.cacheControl.optionMustRevalidate = true;

    // Start parsing from the request line
    m_currentParser = (Memory::Streams::SubParser *) (&clientRequest.requestLine);

    loadDefaultMIMETypes();
}

/**
 * @brief Progresses the parser through different stages of the HTTP request.
 *
 * Transitions from one parsing stage (request line, headers, content) to the next
 * based on the current parser state.
 *
 * @return True if parsing should continue, false otherwise.
 */
bool HTTP::HTTPv1_Server::changeToNextParser()
{
    // Server mode progresses through request line → headers → body.
    if (m_currentParser == &clientRequest.requestLine)
        return changeToNextParserFromClientRequestLine();
    else if (m_currentParser == &clientRequest.headers)
        return changeToNextParserFromClientHeaders();
    else
        return changeToNextParserFromClientContentData();
}

/**
 * @brief Handles parsing after the request line has been fully received.
 *
 * Processes HTTP headers, extracts metadata (host, auth, user-agent, etc.), and
 * determines whether to expect a body or respond immediately.
 *
 * @return True if parsing should continue, false otherwise.
 */
bool HTTP::HTTPv1_Server::changeToNextParserFromClientHeaders()
{
    // Client headers have been received; parse metadata and decide next step.

    // Lambda to parse the Host header and extract host and port information
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

    // Lambda to validate that HTTP version supports Host header in HTTP/1.1+
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

    // Lambda to parse Basic Authentication header
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

    // Lambda to extract User-Agent header
    auto parseUserAgentHeader = [this]()
    {
        // PARSE USER-AGENT
        if (clientRequest.headers.exist("User-Agent"))
            clientRequest.userAgent = clientRequest.headers.getOptionRawStringByName("User-Agent");
    };

    // Lambda to parse Content-Length and Content-Type headers
    auto parseContentLengthAndType = [this](size_t &contentLength)
    {
        // Extract payload size and content type hints from headers.
        contentLength = clientRequest.headers.getOptionAsUINT64("Content-Length");
        string contentType = clientRequest.headers.getOptionValueStringByName("Content-Type");
        if (contentLength)
        {
            clientRequest.content.setTransmitionMode(HTTP::Content::TRANSMIT_MODE_CONTENT_LENGTH);
            if (!clientRequest.content.setContentLengthSize(contentLength))
            {
                // Abort: the advertised length cannot be allocated within limits.
                m_isInvalidHTTPRequest = true;
                serverResponse.status.setCode(HTTP::Status::S_413_PAYLOAD_TOO_LARGE);
            }
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

    // Execute parsing steps
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
            // Allow consumer code to inspect and possibly override headers.
            if (!onClientHeadersReceived())
                m_currentParser = nullptr; // Don't continue with parsing (close the connection)
            else
            {
                if (contentLength)
                {
                    // Expect a body because the client provided Content-Length.
                    m_currentParser = &clientRequest.content;
                }
                else
                {
                    // No body expected, respond now
                    return sendHTTPResponse();
                }
            }
        }
    }
    return true;
}

/**
 * @brief Handles parsing after the request line has been parsed.
 *
 * Validates the HTTP version and URI, then transitions to parsing headers.
 *
 * @return True if parsing should continue, false otherwise.
 */
bool HTTP::HTTPv1_Server::changeToNextParserFromClientRequestLine()
{
    // Request-line parsed; validate URI and HTTP version before reading headers.
    prepareServerVersionOnURI();
    if (m_isInvalidHTTPRequest)
        return sendHTTPResponse();
    else
    {
        if (!onClientURIReceived())
            m_currentParser = nullptr; // Don't continue with parsing.
        else
            m_currentParser = &clientRequest.headers;
    }
    return true;
}

/**
 * @brief Handles parsing of the body content.
 *
 * Currently, streaming bodies are not supported. This method ends parsing
 * and sends the HTTP response.
 *
 * @return Always returns true.
 */
bool HTTP::HTTPv1_Server::changeToNextParserFromClientContentData()
{
    // TODO: Streaming body support not yet implemented
    m_currentParser = nullptr;
    return sendHTTPResponse();
}

/**
 * @brief Sets the HTTP version for the server's response based on client request.
 *
 * Ensures the server responds with a compatible HTTP version.
 */
void HTTP::HTTPv1_Server::prepareServerVersionOnURI()
{
    serverResponse.status.getHTTPVersion()->setMajor(1);
    serverResponse.status.getHTTPVersion()->setMinor(0);

    // Validate major version
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
