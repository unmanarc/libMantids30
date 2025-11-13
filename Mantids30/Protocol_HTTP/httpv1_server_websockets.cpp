#include "httpv1_server.h"
#include "websocket_framecontent.h"
#include <boost/algorithm/string/predicate.hpp>

#include <Mantids30/Helpers/crypto.h>
#include <Mantids30/Helpers/encoders.h>

using namespace std;
using namespace Mantids30::Network::Protocols;
using namespace Mantids30::Network;
using namespace Mantids30;

bool HTTP::HTTPv1_Server::isWebSocketConnectionRequest()
{
    const string connectionHeader = clientRequest.headers.getOptionValueStringByName("CONNECTION");
    const string upgradeHeader = clientRequest.headers.getOptionValueStringByName("UPGRADE");

    return (boost::iequals(connectionHeader,"upgrade") && boost::iequals(upgradeHeader,"websocket"));
}

bool HTTP::HTTPv1_Server::setupAndSendWebSocketHeaderResponse()
{
    // WebSocket magic string as per RFC 6455
    const string WEBSOCKET_MAGIC_STRING = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    // Get the Sec-WebSocket-Key from client
    const string clientKey = clientRequest.headers.getOptionValueStringByName("Sec-WebSocket-Key");

    // Check if key exists and is valid (should be 16 bytes base64 encoded = 24 chars)
    if (clientKey.empty() || clientKey.length() != 24)
    {
        serverResponse.status.setCode(HTTP::Status::S_400_BAD_REQUEST);
        sendFullHTTPResponse();
        return false;
    }

    // Check WebSocket version (we only support version 13 as per RFC 6455)
    const string wsVersion = clientRequest.headers.getOptionValueStringByName("Sec-WebSocket-Version");
    if (wsVersion != "13")
    {
        // Optionally send back supported version
        serverResponse.headers.add("Sec-WebSocket-Version", "13");
        serverResponse.status.setCode(HTTP::Status::S_426_UPGRADE_REQUIRED);
        sendFullHTTPResponse();
        return false;
    }

    // Calculate accept key: base64(sha1(key + magic))
    const string concatenated = clientKey + WEBSOCKET_MAGIC_STRING;
    const string sha1Hash = Helpers::Crypto::calcSHA1(concatenated);
    const string acceptKey = Helpers::Encoders::encodeToBase64(sha1Hash, false);

    // Set response headers for successful WebSocket upgrade
    serverResponse.status.setCode(HTTP::Status::S_101_SWITCHING_PROTOCOLS);

    serverResponse.headers.add("Upgrade", "websocket");
    serverResponse.headers.add("Connection", "Upgrade");
    serverResponse.headers.add("Sec-WebSocket-Accept", acceptKey);

    // Handle optional protocol if requested by client
    const string requestedProtocol = clientRequest.headers.getOptionValueStringByName("Sec-WebSocket-Protocol");
    if (!requestedProtocol.empty())
    {
        // Here you should validate if the requested protocol is supported
        // For now, we'll echo back the first protocol if multiple are requested
        size_t commaPos = requestedProtocol.find(',');
        if (commaPos != string::npos)
        {
            // Multiple protocols requested, select first one
            string selectedProtocol = requestedProtocol.substr(0, commaPos);
            // Trim whitespace if any
            size_t start = selectedProtocol.find_first_not_of(" \t");
            if (start != string::npos)
                selectedProtocol = selectedProtocol.substr(start);
            serverResponse.headers.add("Sec-WebSocket-Protocol", selectedProtocol);
        }
        else
        {
            // Single protocol requested
            serverResponse.headers.add("Sec-WebSocket-Protocol", requestedProtocol);
        }
    }

    // Stream Server HTTP Headers
    serverResponse.immutableHeaders = true;
    return sendHTTPHeadersResponse();
}

bool HTTP::HTTPv1_Server::sendWebSocketData(const char *data, const size_t &len, WebSocket::FrameHeader::OpCode mode)
{
    const size_t MAX_FRAME_SIZE = 65535;
    bool isFinal = false;
    size_t bytesSent = 0;

    do {
        size_t remaining = len - bytesSent;
        size_t frameSize = (remaining > MAX_FRAME_SIZE) ? MAX_FRAME_SIZE : remaining;

        // Check if this is the last frame
        isFinal = (bytesSent + frameSize >= len);

        // Determine opcode: first frame is TEXT, continuation frames are CONTINUATION
        WebSocket::FrameHeader::OpCode opcode = (bytesSent == 0)
                                                    ? mode
                                                    : WebSocket::FrameHeader::OPCODE_CONTINUATION;

        // Create and send frame header
        WebSocket::FrameHeader frmhdr;
        frmhdr.initElemParser(m_streamableObject.get(), false);
        frmhdr.prepareHeader(isFinal, opcode, frameSize, false); // not masked (server to client)
        if (!frmhdr.streamToUpstream())
        {
            return false;
        }

        // Send frame content
        WebSocket::FrameContent frmcontent;
        frmcontent.initElemParser(m_streamableObject.get(), false);
        frmcontent.getContent().get()->append(data + bytesSent, frameSize);
        if (!frmcontent.streamToUpstream())
        {
            return false;
        }

        bytesSent += frameSize;

    } while (bytesSent < len);

    return true;
}



bool HTTP::HTTPv1_Server::changeToNextParserFromWebSocketFrameHeader()
{
    if ( webSocketFrame.content.isFirstFrame() )
    {
        webSocketFrame.frameType = webSocketFrame.header.getOpCode();
        switch (webSocketFrame.frameType)
        {
        case WebSocket::FrameHeader::OPCODE_TEXT:
            webSocketFrame.content.setValidateUtf8(true);
            break;
        case WebSocket::FrameHeader::OPCODE_CONTINUATION:
            // First frame can't be continuation... should specify what type of websocket type is. Drop the connection...
            m_currentSubParser = nullptr;
            return false;
        default:
            // Others... Don´t require manipulation.
            break;
        }
    }

    auto payloadLength = webSocketFrame.header.getPayloadLength();

    // Pass to the content.
    if (payloadLength)
    {
        // Set the data to be absorved by the content.
        bool isMasked = webSocketFrame.header.isMasked();
        webSocketFrame.content.setMasked( isMasked );
        if( isMasked )
        {
            webSocketFrame.content.setMaskingKey( webSocketFrame.header.getMaskingKey() );
        }
        webSocketFrame.content.setPayloadLength( payloadLength );

        // Set the parser to introduce the data into the content.
        m_currentSubParser = &webSocketFrame.content;
    }
    else
    {
        // No payload, call immediatly.
        callOnFinalFragmentReceived();
    }

    return true;
}

bool HTTP::HTTPv1_Server::changeToNextParserFromWebSocketFrameContent()
{
    if (webSocketFrame.header.isFinalFragment())
    {
        return callOnFinalFragmentReceived();
    }
    else
    {
        // Receive again the header:
        webSocketFrame.header.reset();
        m_currentSubParser = &webSocketFrame.header;
        // but the content will be kept in append mode with the previous data.
        return true;
    }
}

bool HTTP::HTTPv1_Server::callOnFinalFragmentReceived()
{
    switch (webSocketFrame.frameType)
    {
    case WebSocket::FrameHeader::OPCODE_CONTINUATION:
        throw std::runtime_error("The first frame should be text/binary/close/ping/pong...");
        break;
    case WebSocket::FrameHeader::OPCODE_TEXT:
        onWebSocketTextFrameReceived();
        break;
    case WebSocket::FrameHeader::OPCODE_BINARY:
        onWebSocketBinaryDataFrameReceived();
        break;
    case WebSocket::FrameHeader::OPCODE_CLOSE:
    {
        onWebSocketConnectionFinished();
        WebSocket::FrameHeader closeHeader;
        closeHeader.initElemParser( m_streamableObject.get() , false );
        closeHeader.prepareCloseFrame(0);
        closeHeader.streamToUpstream();
        webSocketFrame.content.reset();
        webSocketFrame.header.reset();
        m_currentSubParser = nullptr;
        return true;
    } break;
    case WebSocket::FrameHeader::OPCODE_PING:
    {
        WebSocket::FrameHeader pongHeader;
        pongHeader.initElemParser( m_streamableObject.get() , false );
        pongHeader.preparePongFrame(0);
        if (!pongHeader.streamToUpstream())
        {
            // Pong failed! bye and close.
            webSocketFrame.content.reset();
            webSocketFrame.header.reset();
            m_currentSubParser = nullptr;
            return false;
        }
        onWebSocketPingReceived();
    }break;
    case WebSocket::FrameHeader::OPCODE_PONG:
    {
        webSocketFrame.lastPongReceived = time(nullptr);
        onWebSocketPongReceived();
    } break;
    }
    // 0 bytes = Next header...
    // Reset the content.
    webSocketFrame.content.reset();
    webSocketFrame.header.reset();
    m_currentSubParser = &webSocketFrame.header;
    return true;
}

bool HTTP::HTTPv1_Server::sendWebSocketText(const char *data, const size_t &len)
{
    return sendWebSocketData(data,len,WebSocket::FrameHeader::OPCODE_TEXT);
}

bool HTTP::HTTPv1_Server::sendWebSocketBinaryData(const char *data, const size_t &len)
{
    return sendWebSocketData(data,len,WebSocket::FrameHeader::OPCODE_BINARY);
}

bool HTTP::HTTPv1_Server::sendWebSocketPing(const char *data, size_t len)
{
    if (len > 125)
    {
        len = 125;
    }

    WebSocket::FrameHeader pingHeader;
    pingHeader.initElemParser( m_streamableObject.get() , false );
    pingHeader.preparePingFrame(len);
    if (!pingHeader.streamToUpstream())
    {
        return false;
    }

    WebSocket::FrameContent frmcontent;
    frmcontent.initElemParser(m_streamableObject.get(), false);
    frmcontent.getContent().get()->append(data, len);

    return frmcontent.streamToUpstream();
}

