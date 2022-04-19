#include "common_content.h"
#include "common_content_chunked_subparser.h"

#include <limits>

// TODO: CHECK THIS CLASS
using namespace Mantids::Network::HTTP;
using namespace Mantids::Network::HTTP::Common;

using namespace Mantids;

Content::Content()
{
    transmitionMode = TRANSMIT_MODE_CONNECTION_CLOSE;
    currentMode = PROCMODE_CONTENT_LENGTH;
    currentContentLengthSize = 0;
    securityMaxPostDataSize = 17*MB_MULT; // 17Mb intermediate buffer (suitable for 16mb max chunk...).
    securityMaxHttpChunkSize = 16*MB_MULT; // 16mb.
    containerType = CONTENT_TYPE_BIN;
    outStream = &binDataContainer;
    deleteOutStream = false;
}

Content::~Content()
{
    if (deleteOutStream) delete outStream;
}

bool Content::isDefaultStreamableOutput()
{
    return outStream==&binDataContainer || outStream==&urlVars || outStream==&multiPartVars;
}

void Content::setSecurityMaxPostDataSize(const uint64_t &value)
{
    securityMaxPostDataSize = value;
}


Memory::Streams::Parsing::ParseStatus Content::parse()
{
    switch (currentMode)
    {
    case PROCMODE_CHUNK_SIZE:
    {
        size_t targetChunkSize;
        if ((targetChunkSize=parseHttpChunkSize())!=std::numeric_limits<uint32_t>::max())
        {
            if (targetChunkSize>0)
            {
                setParseMode(Memory::Streams::Parsing::PARSE_MODE_SIZE);
                setParseDataTargetSize(targetChunkSize);
                currentMode = PROCMODE_CHUNK_DATA;
                return Memory::Streams::Parsing::PARSE_STAT_GET_MORE_DATA;
            }
            else
            {
                // Done... last chunk.
                // report that is last chunk.
                outStream->writeEOF(true);
                return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
            }
        }
        return Memory::Streams::Parsing::PARSE_STAT_ERROR;
    }
    case PROCMODE_CHUNK_DATA:
    {
        // Ok, continue
        setParseMode(Memory::Streams::Parsing::PARSE_MODE_SIZE);
        setParseDataTargetSize(2); // for CRLF.
        currentMode = PROCMODE_CHUNK_CRLF;
        // Proccess chunk into mem...
        // TODO: validate when outstream is filled up.
        getParsedData()->appendTo(*outStream);
        return Memory::Streams::Parsing::PARSE_STAT_GET_MORE_DATA;
    }
    case PROCMODE_CHUNK_CRLF:
    {
        setParseMode(Memory::Streams::Parsing::PARSE_MODE_DELIMITER);
        setParseDelimiter("\r\n");
        setParseDataTargetSize(1*KB_MULT); // !1kb max (until fails).
        currentMode = PROCMODE_CHUNK_SIZE;
        return Memory::Streams::Parsing::PARSE_STAT_GET_MORE_DATA;
    }
    case PROCMODE_CONTENT_LENGTH:
    {
        // TODO: validate when outstream is filled up.
        getParsedData()->appendTo(*outStream);
        if (getLeftToparse()>0)
        {
            return Memory::Streams::Parsing::PARSE_STAT_GET_MORE_DATA;
        }
        else
        {
            // End of stream reached...
            outStream->writeEOF(true);
            return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
        }
    }
    case PROCMODE_CONNECTION_CLOSE:
    {
        // Content satisfied...
        if (getParsedData()->size())
        {
            // Parsing data...
            // TODO: validate when outstream is filled up.
            getParsedData()->appendTo(*outStream);
            return Memory::Streams::Parsing::PARSE_STAT_GET_MORE_DATA;
        }
        else
        {
            // No more data to parse, ending and processing it...
            outStream->writeEOF(true);
            return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
        }
    }
    }
    return Memory::Streams::Parsing::PARSE_STAT_ERROR;
}

uint32_t Content::parseHttpChunkSize()
{
    uint32_t parsedSize = getParsedData()->toUInt32(16);
    if ( parsedSize == std::numeric_limits<uint32_t>::max() || parsedSize>securityMaxHttpChunkSize)
        return std::numeric_limits<uint32_t>::max();
    else
        return parsedSize;
}

eContent_TransmitionMode Content::getTransmitionMode() const
{
    return transmitionMode;
}

URLVars *Content::getUrlVars()
{
    return &urlVars;
}


eContent_DataType Content::getContainerType() const
{
    return containerType;
}

Memory::Abstract::Vars *Content::postVars()
{
    switch (containerType)
    {
    case CONTENT_TYPE_MIME:
        return &multiPartVars;
    case CONTENT_TYPE_URL:
    case CONTENT_TYPE_BIN:
        return &urlVars; // url vars should be empty here...
    }
    return &urlVars;
}

Network::MIME::MIME_Vars *Content::getMultiPartVars()
{
    return &multiPartVars;
}

void Content::setContainerType(const eContent_DataType &value)
{
    containerType = value;
    if (isDefaultStreamableOutput())
    {
        switch (containerType)
        {
        case CONTENT_TYPE_MIME:
            outStream = &multiPartVars;
            break;
        case CONTENT_TYPE_URL:
            outStream = &urlVars;
            break;
        case CONTENT_TYPE_BIN:
            outStream = &binDataContainer;
            break;
        }
    }
}

void Content::setSecurityMaxHttpChunkSize(const uint32_t &value)
{
    securityMaxHttpChunkSize = value;
}

bool Content::stream(Memory::Streams::Status & wrStat)
{
    // Act as a client. Send data from here.
    switch (transmitionMode)
    {
    case TRANSMIT_MODE_CHUNKS:
    {
        Content_Chunked_SubParser retr(upStream);
        return outStream->streamTo(&retr,wrStat) && upStream->getFailedWriteState()==0;
    }
    case TRANSMIT_MODE_CONTENT_LENGTH:
    case TRANSMIT_MODE_CONNECTION_CLOSE:
    {
        return outStream->streamTo(upStream, wrStat) && upStream->getFailedWriteState()==0;
    }
    }
    return true;
}

void Content::setTransmitionMode(const eContent_TransmitionMode &value)
{
    transmitionMode = value;

    switch (transmitionMode)
    {
    case TRANSMIT_MODE_CONNECTION_CLOSE:
    {
        // TODO: disable connection-close for client->server
        setParseMode(Memory::Streams::Parsing::PARSE_MODE_DIRECT);
        setParseDataTargetSize(securityMaxPostDataSize); // !1kb max (until fails).
        currentMode = PROCMODE_CONNECTION_CLOSE;
    }break;
    case TRANSMIT_MODE_CHUNKS:
    {
        // TODO: disable chunk transmition for client->server
        setParseMode(Memory::Streams::Parsing::PARSE_MODE_DELIMITER);
        setParseDelimiter("\r\n");
        setParseDataTargetSize(64); // 64 bytes max (until fails).
        currentMode = PROCMODE_CHUNK_SIZE;
    }break;
    case TRANSMIT_MODE_CONTENT_LENGTH:
    {
        setParseMode(Memory::Streams::Parsing::PARSE_MODE_DIRECT);
        currentMode = PROCMODE_CONTENT_LENGTH;
    }break;
    }
}

bool Content::setContentLenSize(const uint64_t &contentLengthSize)
{
    if (contentLengthSize>securityMaxPostDataSize)
    {
        // The content length specified in the
        currentContentLengthSize = 0;
        setParseDataTargetSize(0);
        return false;
    }

    currentContentLengthSize=contentLengthSize;
    setParseDataTargetSize(contentLengthSize);

    return true;
}

void Content::setMaxPostSizeInMemBeforeGoingToFS(const uint64_t &value)
{
    //getParsedData()->setMaxContainerSizeUntilGoingToFS(value);
    binDataContainer.setMaxContainerSizeUntilGoingToFS(value);
}

void Content::useFilesystem(const std::string &fsFilePath)
{
    // TODO:
    //binDataContainer.setFsBaseFileName();
}

void Content::preemptiveDestroyStreamableOuput()
{
    binDataContainer.clear();
    if (this->deleteOutStream)
    {
        this->deleteOutStream = false;
        delete outStream;
        outStream = &binDataContainer;
    }
}

Memory::Streams::Streamable *Content::getStreamableOuput()
{
    return outStream;
}

void Content::setStreamableOutput(Memory::Streams::Streamable *outDataContainer, bool deleteOutStream)
{
    // This stream has been setted up before...
    // Delete the previous stream/data before replacing...
    preemptiveDestroyStreamableOuput();

    this->deleteOutStream = deleteOutStream;
    this->outStream = outDataContainer;

    if (this->outStream == nullptr)
    {
        // Point to empty container...
        outStream = &binDataContainer;
        this->deleteOutStream = false;
    }
}

uint64_t Content::getStreamSize()
{
    return outStream->size();
}

