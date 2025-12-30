#include "parser.h"
#include <memory>
#include <openssl/bio.h>
#include <optional>

using namespace Mantids30::Memory::Streams;

Parser::Parser(std::shared_ptr<StreamableObject> value, bool clientMode)
{
    this->m_clientMode = clientMode;
    this->m_streamableObject = value;
}

void Parser::parseObject(Parser::ErrorMSG *err)
{
    *err = PARSING_SUCCEED;

    this->writeStatus.finished = false;

    // Call the init protocol...
    m_initialized = initProtocol();

    // If initialized ok...
    if (m_initialized)
    {
        // the streamable object introduced on the object creation is streamed to this.
        // write function will write....

        bool succeed = true;

        if (m_preStreamableObject)
        {
            succeed = m_preStreamableObject->streamTo(this);
        }
        if (succeed)
        {
            succeed = m_streamableObject->streamTo(this);
        }

        // Protocol Failed Somewhere...
        if (!succeed)
        {
            *err = PARSING_ERR_PARSING;
        }

        // Call: Finish the protocol (either failed or not).
        endProtocol();
    }
    else
    {
        // status: failed.
        *err = PARSING_ERR_INIT;
    }
}

std::optional<size_t> Parser::write(const void *buf, const size_t &count)
{
    // Parse this data...
    size_t ttl = 0;

    if (count == 0)
    {
        // EOF:
        size_t ttl = 0;
        std::optional<size_t> r = parseData("", 0, &ttl);

        if (r == std::nullopt && (parsingDebugOptions & PARSING_DEBUG_PRINT_FAILED_STATUS))
        {
            fprintf(stderr, "\033[31m[PARSING_ERROR] - %p -  During protocol finalization (Write EOF) for parser\033[0m\n", this);
            fflush(stderr);
        }

        return r;
    }

    if (parsingDebugOptions & PARSING_DEBUG_PRINT_DATA_PARSED)
    {
        fprintf(stderr, "[PARSING_DEBUG] - %p - Writting %lu bytes on parser (Current SubParser: %s)\n", this, count,
                (!m_currentSubParser ? "nullptr" : m_currentSubParser->getSubParserName().c_str()));
        fflush(stderr);
        BIO_dump_fp(stderr, static_cast<const char *>(buf), count);
        fflush(stderr);
    }

    // The content streamed is parsed here:
    std::optional<size_t> r = parseData(buf, count, &ttl);

    if (parsingDebugOptions & PARSING_DEBUG_PRINT_DATA_PARSED)
    {
        if (r.has_value())
        {
            fprintf(stderr, "[PARSING_DEBUG] - %p - %ld bytes written on parser (Current SubParser: %s)\n", this, r.value(),
                    (!m_currentSubParser ? "nullptr" : m_currentSubParser->getSubParserName().c_str()));
            fflush(stderr);
        }
    }

    if (r == std::nullopt && (parsingDebugOptions & PARSING_DEBUG_PRINT_FAILED_STATUS))
    {
        fprintf(stderr, "\033[31m[PARSING_ERROR] - %p -  During protocol data streaming\033[0m\n", this);
        fflush(stderr);
    }

    return r;
}

std::optional<size_t> Parser::parseData(const void *buf, size_t count, size_t *ttl)
{
    if (*ttl > m_maxTTL)
    {
        // TODO: reset TTL?
        if (parsingDebugOptions & PARSING_DEBUG_PRINT_FAILED_STATUS)
        {
            fprintf(stderr, "\033[31m[PARSING_ERROR] - %p - Parser reaching TTL %zu\033[0m\n", this, *ttl);
            fflush(stderr);
        }

        return std::nullopt; // TTL Reached...
    }
    (*ttl)++;

    // We are parsing data here...
    // written bytes will be filled with first=error, and second=displacebytes
    // displace bytes is the number of bytes that the subparser have taken from the incoming buffer, so we have to displace them.
    std::optional<size_t> writtenBytes = 0;

    // The m_currentParser is a subparser...
    if (m_currentSubParser != nullptr)
    {
        // Default state: get more data...
        m_currentSubParser->setParseStatus(SubParser::PARSE_GET_MORE_DATA);

        // Here, the parser should call the sub stream parser parse function and set the new status.
        writtenBytes = m_currentSubParser->writeIntoSubParser(buf, count);

        // Failed to write this into the parser...
        if (writtenBytes == std::nullopt)
        {
            if (parsingDebugOptions & PARSING_DEBUG_PRINT_FAILED_STATUS)
            {
                fprintf(stderr, "\033[31m[PARSING_ERROR] - %p - Failed to write data into parser %s\033[0m\n", this, (!m_currentSubParser ? "nullptr" : m_currentSubParser->getSubParserName().c_str()));
                fflush(stderr);

                fflush(stderr);
                BIO_dump_fp(stderr, static_cast<const char *>(buf), count);
                fflush(stderr);
            }

            return std::nullopt;
        }

        // TODO: what if error? how to tell the parser that it should analize the connection up to there (without correctness).
        switch (m_currentSubParser->getParseStatus())
        {
        case SubParser::PARSE_GOTO_NEXT_SUBPARSER:
        {
            if (parsingDebugOptions & PARSING_DEBUG_PRINT_INTERNAL_DYNAMICS)
            {
                fprintf(stderr, "[PARSING_INTERNAL] - %p - PARSE_GOTO_NEXT_SUBPARSER requested from %s\n", this, (!m_currentSubParser ? "nullptr" : m_currentSubParser->getSubParserName().c_str()));
                fflush(stderr);
            }

            std::string previousParser = (!m_currentSubParser ? "nullptr" : m_currentSubParser->getSubParserName().c_str());

            // Check if there is next parser...
            if (!changeToNextParser())
            {
                if (parsingDebugOptions & PARSING_DEBUG_PRINT_FAILED_STATUS)
                {
                    fprintf(stderr, "\033[31m[PARSING_ERROR] - %p - Subprotocol %s failed to change to the next subparser\033[0m\n", this,
                            (!m_currentSubParser ? "nullptr" : m_currentSubParser->getSubParserName().c_str()));
                    fflush(stderr);
                }

                return std::nullopt;
            }

            if (parsingDebugOptions & PARSING_DEBUG_PRINT_INTERNAL_DYNAMICS)
            {
                fprintf(stderr, "[PARSING_INTERNAL] - %p - PARSE_GOTO_NEXT_SUBPARSER changed to %s\n", this, (!m_currentSubParser ? "nullptr" : m_currentSubParser->getSubParserName().c_str()));
                fflush(stderr);
            }

            // If the parser is changed to nullptr, then the connection is ended (-2).
            // Parsed OK :)... Pass to the next stage
            if (m_currentSubParser == nullptr)
                this->writeStatus.finished = true;
            if (m_currentSubParser == nullptr || writtenBytes.value() == count)
                return writtenBytes;
        }
        break;
        case SubParser::PARSE_GET_MORE_DATA:
        {
            if (parsingDebugOptions & PARSING_DEBUG_PRINT_INTERNAL_DYNAMICS)
            {
                fprintf(stderr, "[PARSING_INTERNAL] - %p - PARSE_GET_MORE_DATA requested from %s\n", this, (!m_currentSubParser ? "nullptr" : m_currentSubParser->getSubParserName().c_str()));
                fflush(stderr);
            }
            // More data required... (TODO: check this)
            if (writtenBytes.value() == count)
                return writtenBytes;
        }
        break;
            // Bad parsing... end here.
        case SubParser::PARSE_ERROR:
            if (parsingDebugOptions & PARSING_DEBUG_PRINT_FAILED_STATUS)
            {
                fprintf(stderr, "\033[31m[PARSING_ERROR] - %p - PARSE_STAT_ERROR executed from %s\033[0m\n", this, (!m_currentSubParser ? "nullptr" : m_currentSubParser->getSubParserName().c_str()));
                fflush(stderr);
            }

            return std::nullopt;
            // Unknown parser...
        }
    }
    else
    {
        if (parsingDebugOptions & PARSING_DEBUG_PRINT_FAILED_STATUS)
        {
            fprintf(stderr, "\033[31m[PARSING_OOB] - %p - Parsing was finished and we are trying to write more data here.\033[0m\n", this);
            fflush(stderr);
            BIO_dump_fp(stderr, static_cast<const char *>(buf), count);
            fflush(stderr);
        }
        return std::nullopt;
    }

    // TODO: what if writtenBytes == 0?
    // Data left in buffer, process it.
    if (writtenBytes.value() != count)
    {
        buf = (static_cast<const char *>(buf)) + writtenBytes.value();
        count -= writtenBytes.value();

        // Data left to process.. (Recursive call)
        std::optional<size_t> x = parseData(buf, count, ttl);

        if (x == std::nullopt)
            return std::nullopt;

        return x.value() + writtenBytes.value();
    }
    else
        return writtenBytes;
}

void Parser::initSubParser(SubParser *subparser)
{
    // Parsers are initialized with the pointer from the smart pointer.
    // all subparser should live in the context of the parser, and never run after the parser dies...
    subparser->initElemParser((m_streamableObject != nullptr) ? m_streamableObject.get() : this, m_clientMode);
}

void Parser::setMaxTTL(const size_t &value)
{
    m_maxTTL = value;
}

void Parser::setPreStreamableObject(const std::shared_ptr<Memory::Streams::StreamableObject> &newPreStreamableObject)
{
    m_preStreamableObject = newPreStreamableObject;
}

void Parser::setStreamable(std::shared_ptr<Memory::Streams::StreamableObject> value)
{
    m_streamableObject = value;
}
