#include "parser.h"
#include <memory>
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

    // Call the init protocol...
    m_initialized = initProtocol();

    // If initialized ok...
    if (m_initialized)
    {
        // the streamable object introduced on the object creation is streamed to this.
        // write function will write....
        bool streamedBytes = m_streamableObject->streamTo(this);

        // Protocol Failed Somewhere...
        if (!streamedBytes)
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
    bool finished = false;
    size_t r;

    if (count == 0)
    {
        // EOF:
        size_t ttl = 0;
        bool finished = false;

        std::optional<size_t> r = parseData("", 0, &ttl, &finished);
        return !finished? std::nullopt : r;
    }

#ifdef DEBUG_PARSER
    printf("Writting on Parser %p:\n", this);
    fflush(stdout);
    BIO_dump_fp(stdout, static_cast<char *>(buf), count);
    fflush(stdout);
#endif

    // The content streamed is parsed here:
    return parseData(buf, count, &ttl, &finished);
}

std::optional<size_t> Parser::parseData(const void *buf, size_t count, size_t *ttl, bool *finished)
{
    if (*ttl > m_maxTTL)
    {
        // TODO: reset TTL?
#ifdef DEBUG_PARSER
        fprintf(stderr, "%p Parser reaching TTL %zu", this, *ttl);
        fflush(stderr);
#endif
        return std::nullopt; // TTL Reached...
    }
    (*ttl)++;

    // We are parsing data here...
    // written bytes will be filled with first=error, and second=displacebytes
    // displace bytes is the number of bytes that the subparser have taken from the incoming buffer, so we have to displace them.
    std::optional<size_t> writtenBytes = 0;

    // The m_currentParser is a subparser...
    if (m_currentParser != nullptr)
    {
        // Default state: get more data...
        m_currentParser->setParseStatus(SubParser::PARSE_GET_MORE_DATA);

        // Here, the parser should call the sub stream parser parse function and set the new status.
        writtenBytes = m_currentParser->writeIntoParser(buf, count);

        // Failed to write this into the parser...
        if (writtenBytes == std::nullopt)
            return std::nullopt;

        // TODO: what if error? how to tell the parser that it should analize the connection up to there (without correctness).
        switch (m_currentParser->getParseStatus())
        {
        case SubParser::PARSE_GOTO_NEXT_SUBPARSER:
        {
#ifdef DEBUG_PARSER
            printf("%p PARSE_GOTO_NEXT_SUBPARSER requested from %s\n", this, (!m_currentParser ? "nullptr" : m_currentParser->getSubParserName().c_str()));
            fflush(stdout);
#endif
            // Check if there is next parser...
            if (!changeToNextParser())
                return std::nullopt;
#ifdef DEBUG_PARSER
            printf("%p PARSE_GOTO_NEXT_SUBPARSER changed to %s\n", this, (!m_currentParser ? "nullptr" : m_currentParser->getSubParserName().c_str()));
            fflush(stdout);
#endif
            // If the parser is changed to nullptr, then the connection is ended (-2).
            // Parsed OK :)... Pass to the next stage
            if (m_currentParser == nullptr)
                *finished = true;
            if (m_currentParser == nullptr || writtenBytes.value() == count)
                return writtenBytes;
        }
        break;
        case SubParser::PARSE_GET_MORE_DATA:
        {
#ifdef DEBUG_PARSER
            printf("%p PARSE_GET_MORE_DATA requested from %s\n", this, (!m_currentParser ? "nullptr" : m_currentParser->getSubParserName().c_str()));
            fflush(stdout);
#endif
            // More data required... (TODO: check this)
            if (writtenBytes.value() == count)
                return writtenBytes;
        }
        break;
            // Bad parsing... end here.
        case SubParser::PARSE_ERROR:
#ifdef DEBUG_PARSER
            printf("%p PARSE_STAT_ERROR executed from %s\n", this, (!m_currentParser ? "nullptr" : m_currentParser->getSubParserName().c_str()));
            fflush(stdout);
#endif

            return std::nullopt;
            // Unknown parser...
        }
    }

    // TODO: what if writtenBytes == 0?
    // Data left in buffer, process it.
    if (writtenBytes.value() != count)
    {
        buf = (static_cast<const char *>(buf)) + writtenBytes.value();
        count -= writtenBytes.value();

        // Data left to process.. (Recursive call)
        std::optional<size_t> x = parseData(buf, count, ttl, finished);

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

void Parser::setStreamable(std::shared_ptr<Memory::Streams::StreamableObject> value)
{
    m_streamableObject = value;
}
