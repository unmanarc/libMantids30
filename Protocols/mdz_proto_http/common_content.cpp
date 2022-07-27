#include "common_content.h"
#include "common_content_chunked_subparser.h"

#include <limits>

// TODO: CHECK THIS CLASS
using namespace Mantids::Protocols::HTTP;
using namespace Mantids::Protocols::HTTP::Common;

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
    subParserName = "Content";
}

Content::~Content()
{
    if (deleteOutStream) delete outStream;
}

bool Content::isDefaultStreamableObj()
{
    return outStream==&binDataContainer || outStream==&urlPostVars || outStream==&multiPartVars;
}

void Content::setSecurityMaxPostDataSize(const uint64_t &value)
{
    securityMaxPostDataSize = value;
}


Memory::Streams::SubParser::ParseStatus Content::parse()
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
                    setParseMode(Memory::Streams::SubParser::PARSE_MODE_SIZE);
                    setParseDataTargetSize(targetChunkSize);
                    currentMode = PROCMODE_CHUNK_DATA;
                    return Memory::Streams::SubParser::PARSE_STAT_GET_MORE_DATA;
                }
                else
                {
                    // Done... last chunk.
                    // report that is last chunk.
                    outStream->writeEOF(true);
                    return Memory::Streams::SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER;
                }
            }
            return Memory::Streams::SubParser::PARSE_STAT_ERROR;
        }
        case PROCMODE_CHUNK_DATA:
        {
            // Ok, continue
            setParseMode(Memory::Streams::SubParser::PARSE_MODE_SIZE);
            setParseDataTargetSize(2); // for CRLF.
            currentMode = PROCMODE_CHUNK_CRLF;
            // Proccess chunk into mem...
            // TODO: validate when outstream is filled up.
            getParsedBuffer()->appendTo(*outStream);
            return Memory::Streams::SubParser::PARSE_STAT_GET_MORE_DATA;
        }
        case PROCMODE_CHUNK_CRLF:
        {
            setParseMode(Memory::Streams::SubParser::PARSE_MODE_DELIMITER);
            setParseDelimiter("\r\n");
            setParseDataTargetSize(1*KB_MULT); // !1kb max (until fails).
            currentMode = PROCMODE_CHUNK_SIZE;
            return Memory::Streams::SubParser::PARSE_STAT_GET_MORE_DATA;
        }
        case PROCMODE_CONTENT_LENGTH:
        {
            // TODO: validate when outstream is filled up.
            getParsedBuffer()->appendTo(*outStream);
            if (getLeftToparse()>0)
            {
#ifdef DEBUG
                printf("%p Content PROCMODE_CONTENT_LENGTH - left to parse: %lu\n",this, getLeftToparse());
#endif
                return Memory::Streams::SubParser::PARSE_STAT_GET_MORE_DATA;
            }
            else
            {
#ifdef DEBUG
                printf("%p Content PROCMODE_CONTENT_LENGTH - EOS\n",this);
#endif
                // End of stream reached...
                outStream->writeEOF(true);
                return Memory::Streams::SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER;
            }
        }
        case PROCMODE_CONNECTION_CLOSE:
        {
            // Content satisfied...
            if (getParsedBuffer()->size())
            {
                // Parsing data...
                // TODO: validate when outstream is filled up.
                getParsedBuffer()->appendTo(*outStream);
                return Memory::Streams::SubParser::PARSE_STAT_GET_MORE_DATA;
            }
            else
            {
                // No more data to parse, ending and processing it...
                outStream->writeEOF(true);
                return Memory::Streams::SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER;
            }
        }
    }
    return Memory::Streams::SubParser::PARSE_STAT_ERROR;
}

uint32_t Content::parseHttpChunkSize()
{
    uint32_t parsedSize = getParsedBuffer()->toUInt32(16);
    if ( parsedSize == std::numeric_limits<uint32_t>::max() || parsedSize>securityMaxHttpChunkSize)
        return std::numeric_limits<uint32_t>::max();
    else
        return parsedSize;
}

Content::eTransmitionMode Content::getTransmitionMode() const
{
    return transmitionMode;
}

URLVars *Content::getUrlPostVars()
{
    if (containerType == CONTENT_TYPE_URL)
        return &urlPostVars;
    throw std::runtime_error("Don't call getUrlPostVars when the content is not URL.");
}

Protocols::MIME::MIME_Message *Content::getMultiPartVars()
{
    if ( containerType != CONTENT_TYPE_MIME )
        throw std::runtime_error("Don't call getMultiPartVars when the content is not MIME.");
    return &multiPartVars;
}

Content::eDataType Content::getContainerType() const
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
        return &urlPostVars; // url vars should be empty here...
    }
    return &urlPostVars;
}



void Content::setContainerType(const eDataType &value)
{
    containerType = value;
    if (isDefaultStreamableObj())
    {
        switch (containerType)
        {
        case CONTENT_TYPE_MIME:
            outStream = &multiPartVars;
            break;
        case CONTENT_TYPE_URL:
            outStream = &urlPostVars;
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

bool Content::stream(Memory::Streams::StreamableObject::Status & wrStat)
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

void Content::setTransmitionMode(const eTransmitionMode &value)
{
    transmitionMode = value;

    switch (transmitionMode)
    {
    case TRANSMIT_MODE_CONNECTION_CLOSE:
    {
        // TODO: disable connection-close for client->server
        setParseMode(Memory::Streams::SubParser::PARSE_MODE_DIRECT);
        setParseDataTargetSize(securityMaxPostDataSize); // !1kb max (until fails).
        currentMode = PROCMODE_CONNECTION_CLOSE;
    }break;
    case TRANSMIT_MODE_CHUNKS:
    {
        // TODO: disable chunk transmition for client->server
        setParseMode(Memory::Streams::SubParser::PARSE_MODE_DELIMITER);
        setParseDelimiter("\r\n");
        setParseDataTargetSize(64); // 64 bytes max (until fails).
        currentMode = PROCMODE_CHUNK_SIZE;
    }break;
    case TRANSMIT_MODE_CONTENT_LENGTH:
    {
        setParseMode(Memory::Streams::SubParser::PARSE_MODE_DIRECT);
        currentMode = PROCMODE_CONTENT_LENGTH;
    }break;
    }
}

bool Content::setContentLenSize(const uint64_t &contentLengthSize)
{
    if (contentLengthSize>securityMaxPostDataSize)
    {
        // Can't receive this data...
        currentContentLengthSize = 0;
        setParseDataTargetSize(0);
        return false;
    }

    // The content length specified in the header
    currentContentLengthSize=contentLengthSize;
    setParseDataTargetSize(contentLengthSize);

    return true;
}

void Content::setMaxPostSizeInMemBeforeGoingToFS(const uint64_t &value)
{
    //getParsedBuffer()->setMaxContainerSizeUntilGoingToFS(value);
    binDataContainer.setMaxContainerSizeUntilGoingToFS(value);
}
/*
void Content::useFilesystem(const std::string &fsFilePath)
{
    // TODO:
    //binDataContainer.setFsBaseFileName();
}
*/
void Content::preemptiveDestroyStreamableObj()
{
    binDataContainer.clear();
    if (this->deleteOutStream)
    {
        this->deleteOutStream = false;
        delete outStream;
        outStream = &binDataContainer;
    }
}

Memory::Streams::StreamableObject *Content::getStreamableObj()
{
    return outStream;
}

void Content::setStreamableObj(Memory::Streams::StreamableObject *outDataContainer, bool deleteOutStream)
{
    // This stream has been setted up before...
    // Delete the previous stream/data before replacing...
    preemptiveDestroyStreamableObj();

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

