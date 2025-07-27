#include "subparser.h"
#include "b_ref.h"
#include <optional>

using namespace Mantids30::Memory::Streams;

void SubParser::initElemParser(Memory::Streams::StreamableObject *upStreamObj, bool clientMode)
{
    this->m_upStream = upStreamObj;
    this->m_clientMode = clientMode;
}

std::optional<size_t> SubParser::writeIntoParser(const void *buf, size_t count)
{
    if (!count)
    {
        m_streamEnded = true;
    }

    switch (m_parseMode)
    {
    case PARSE_MODE_DELIMITER:

#ifdef DEBUG_PARSER
        printf("Parse by delimiter %p\n", this);
        fflush(stdout);
        BIO_dump_fp(stdout, static_cast<char *>(buf), count);
#endif
        return parseByDelimiter(buf, count);
    case PARSE_MODE_SIZE:
#ifdef DEBUG_PARSER
        printf("Parse by size %p\n", this);
        fflush(stdout);
        BIO_dump_fp(stdout, static_cast<char *>(buf), count);
#endif
        return parseBySize(buf, count);
    case PARSE_MODE_VALIDATOR:
#ifdef DEBUG_PARSER
        printf("Parse by validator %p\n", this);
        fflush(stdout);
        BIO_dump_fp(stdout, static_cast<char *>(buf), count);
#endif
        return parseByValidator(buf, count);
    case PARSE_MODE_DIRECT_DELIMITER:
#ifdef DEBUG_PARSER
        printf("Parse by direct delimiter %p\n", this);
        fflush(stdout);
        BIO_dump_fp(stdout, static_cast<char *>(buf), count);
#endif
        return parseDirectDelimiter(buf, count);
    case PARSE_MODE_CONNECTION_END:
#ifdef DEBUG_PARSER
        printf("Parse by connection end %p\n", this);
        fflush(stdout);
        BIO_dump_fp(stdout, static_cast<char *>(buf), count);
#endif
        return parseByConnectionEnd(buf, count);
    case PARSE_MODE_MULTIDELIMITER:
#ifdef DEBUG_PARSER
        printf("Parse by multidelimiter %p\n", this);
        fflush(stdout);
        BIO_dump_fp(stdout, static_cast<char *>(buf), count);
#endif
        return parseByMultiDelimiter(buf, count);
    case PARSE_MODE_DIRECT:
#ifdef DEBUG_PARSER
        printf("Parse by direct %p\n", this);
        fflush(stdout);
        BIO_dump_fp(stdout, static_cast<char *>(buf), count);
#endif
        return parseDirect(buf, count);
    default:
        break;
    }
    return std::nullopt;
}

size_t SubParser::ParseValidator(Mantids30::Memory::Containers::B_Base &)
{
    return std::numeric_limits<size_t>::max();
}

SubParser::ParseStatus SubParser::getParseStatus() const
{
    return m_parseStatus;
}

void SubParser::setParseStatus(const ParseStatus &value)
{
    m_parseStatus = value;
}

void SubParser::setParseDelimiter(const std::string &value)
{
    m_parseDelimiter = value;
}

Mantids30::Memory::Containers::B_Base *SubParser::getParsedBuffer()
{
    return &m_parsedBuffer;
}


std::optional<size_t> SubParser::appendToUnparsedBuffer(const void *buf, size_t count)
{
    std::optional<size_t> bytesAppended = 0;
    size_t leftSize = m_unparsedBuffer.getSizeLeft();

    // Overflow of the subparser buffer...
    if (!leftSize && count>0)
    {
#ifdef DEBUG_PARSER
        printf("%p Subparser unparsed buffer size exceeded. can't continue with the parsing....\n", this);
        fflush(stdout);
#endif
        return std::nullopt;
    }

    // stick to the unparsedBuffer object size.
    count = std::min(count, leftSize);

    // There is some data to add...
    if (count > 0)
    {
        bytesAppended = m_unparsedBuffer.append(buf, count);
        if (bytesAppended==std::nullopt || bytesAppended.value() != count)
        {
            // Failed to append this data (weird...)
            m_unparsedBuffer.clear();
            return std::nullopt;
        }
    }

    return bytesAppended;
}

std::optional<size_t> SubParser::parseByMultiDelimiter(const void *buf, size_t count)
{
    size_t prevSize = m_unparsedBuffer.size(), bytesToDisplace = 0;
    std::optional<size_t> needlePos = std::nullopt, bytesAppended = appendToUnparsedBuffer(buf,count);

    // Error appending data...
    if (bytesAppended == std::nullopt)
        return std::nullopt;

    bytesToDisplace = bytesAppended.value();
    m_parsedBuffer.reference(&m_unparsedBuffer);

    // Attempt to find the delimiter in the unparsed buffer
    needlePos = m_unparsedBuffer.find(m_parseMultiDelimiter, m_delimiterFound);
    if (needlePos != std::nullopt)
    {
        // Delimiter found, update parsed buffer and status
        m_parsedBuffer.reference(&m_unparsedBuffer, 0, needlePos.value());

#ifdef DEBUG_PARSER
        printf("Parsing by multidelimiter: %s\n", m_parsedBuffer.toString().c_str());
        fflush(stdout);
#endif

        setParseStatus(parse());
        m_unparsedBuffer.clear();

        // Calculate bytes to displace
        // We may have copied in the unparsed buffer more than we need to deliver. so
        // the unparsed buffer is destroyed and you only need to displace the useful bytes.
        bytesToDisplace = needlePos.value() + m_delimiterFound.size() - prevSize;
    }
    else if (m_streamEnded)
    {
        // No delimiter found but stream has ended, parse what's available
        m_parsedBuffer.reference(&m_unparsedBuffer);
        setParseStatus(parse());
        m_unparsedBuffer.clear();
    }
    // else: delimiter not found yet, keep buffering

    return bytesToDisplace;
}

std::optional<size_t> SubParser::parseByDelimiter(const void *buf, size_t count)
{
    size_t prevSize = m_unparsedBuffer.size(), bytesToDisplace = 0;
    std::optional<size_t> needlePos, bytesAppended = appendToUnparsedBuffer(buf,count);

    // Error appending data...
    if (bytesAppended == std::nullopt)
        return std::nullopt;

    bytesToDisplace = bytesAppended.value();
    m_parsedBuffer.reference(&m_unparsedBuffer);

#ifdef DEBUG_PARSER
    auto x = m_unparsedBuffer.toString();
    printf("%p Searching delimiter...\n", this);
    fflush(stdout);
    BIO_dump_fp(stdout, (char *) parseDelimiter.c_str(), m_parseDelimiter.size());
    BIO_dump_fp(stdout, (char *) x.c_str(), x.size());
#endif

    needlePos = m_unparsedBuffer.find(m_parseDelimiter.c_str(), m_parseDelimiter.size());
    if (needlePos!=std::nullopt)
    {
        // needle found.
        m_parsedBuffer.reference(&m_unparsedBuffer, 0, needlePos.value());

#ifdef DEBUG_PARSER
        printf("Parsing by delimiter: %s\n", m_parsedBuffer.toString().c_str());
        fflush(stdout);
#endif
        setParseStatus(parse());
        m_unparsedBuffer.clear();

        // Bytes to displace:
        bytesToDisplace = (needlePos.value() - prevSize) + m_parseDelimiter.size();
#ifdef DEBUG_PARSER
        printf("%p Delimiter found and parsed -> displaced %lu bytes\n", this, bytesToDisplace);
        fflush(stdout);
#endif
    }
    else if (m_streamEnded)
    {
        m_parsedBuffer.reference(&m_unparsedBuffer);
#ifdef DEBUG_PARSER
        printf("%p Delimiter not found and parsed, displaced %lu bytes\n", this, bytesToDisplace);
        fflush(stdout);
#endif
        setParseStatus(parse());
        m_unparsedBuffer.clear();
    }

#ifdef DEBUG_PARSER
    printf("%p displacing %lu bytes\n", this, bytesToDisplace);
    fflush(stdout);
#endif

    return bytesToDisplace;
}

std::optional<size_t> SubParser::parseBySize(const void *buf, size_t count)
{
    if (count == 0)
    {
        // EOF.
        // Abort current subparser because we did not match the requested size.
        setParseStatus(PARSE_GOTO_NEXT_SUBPARSER);
        m_unparsedBuffer.clear(); // Destroy the container data.
        return 0;
    }

    std::optional<size_t> bytesAppended = appendToUnparsedBuffer(buf,count);
    size_t bytesToDisplace = 0;

    // Error appending data...
    if (bytesAppended == std::nullopt)
        return std::nullopt;

    // will only displace the bytes appended to the buffer.
    // because the buffer can only handle the max requested size.
    bytesToDisplace = bytesAppended.value();

    // Ready...
    if (m_unparsedBuffer.getSizeLeft() == 0)
    {
        m_parsedBuffer.reference(&m_unparsedBuffer);
#ifdef DEBUG_PARSER
        printf("Parsing by size: %s\n", m_parsedBuffer.toString().c_str());
        fflush(stdout);
#endif
        setParseStatus(parse());
        m_unparsedBuffer.clear(); // Destroy the container data.
    }

    return bytesToDisplace;
}

std::optional<size_t> SubParser::parseByValidator(const void *, size_t)
{
    // TODO: parseByValidator
    /*B_MEM bc;

    size_t origSize = postParsedBuffer.getContainerSize();

    bc.setMaxSize(postParsedBuffer.getMaxSize());
    postParsedBuffer.appendTo(bc);
    if (bc.append(buf,count)!=count) 
        return -1;
    size_t validatedBytes = ParseValidator(bc);
    if (validatedBytes == -1)
    {
        // NOT FOUND:
        if (postParsedBuffer.append(buf,count)!=count) 
        return -1;
        return count;
    }

    size_t bytesLeft = bc.getContainerSize()-validatedBytes;
    */
    return std::nullopt;
}

std::optional<size_t> SubParser::parseByConnectionEnd(const void *buf, size_t count)
{
    std::optional<size_t> bytesAppended = appendToUnparsedBuffer(buf,count);
    size_t bytesToDisplace = 0;

    // Error appending data...
    if (bytesAppended == std::nullopt)
        return std::nullopt;

    bytesToDisplace = bytesAppended.value();

    if (count == 0)
    {
        // !count means EOF. so, the parse will be done on EOF:

        m_parsedBuffer.reference(&m_unparsedBuffer);

#ifdef DEBUG_PARSER
        printf("Parsing by connection end: %s\n", m_parsedBuffer.toString().c_str());
        fflush(stdout);
#endif

        setParseStatus(parse()); // analyze on connection end.
        m_unparsedBuffer.clear(); // Destroy the container data.
        return 0;
    }

    return bytesToDisplace;
}


/*

  //  std::optional<size_t> copiedDirect;

    // TODO: check.
    // stick to the unparsedBuffer object size.
    count = std::min(count, m_unparsedBuffer.getSizeLeft());

    // TODO: termination?
*/
std::optional<size_t> SubParser::parseDirect(const void *buf, size_t count)
{
    std::optional<size_t> bytesAppended = appendToUnparsedBuffer(buf,count);
    size_t bytesToDisplace = 0;

    // Error appending data...
    if (bytesAppended == std::nullopt)
        return std::nullopt;

    bytesToDisplace = bytesAppended.value();

    // Parse it...
    m_parsedBuffer.reference(&m_unparsedBuffer);

#ifdef DEBUG_PARSER
    printf("Parsing direct (%lu, until size %lu): %s\n", m_parsedBuffer.size(), static_cast<uint64_t>(0), m_parsedBuffer.toString().c_str());
    fflush(stdout);
#endif

    setParseStatus(parse());

    // All the unparsed buffer was consumed by parsed buffer to the next parser...
    size_t curBufSize = m_unparsedBuffer.size();
    m_unparsedBuffer.clear(); // Reset the container data for the next element.
    m_unparsedBuffer.reduceMaxSizeBy(curBufSize);

    return bytesToDisplace;
}

std::optional<size_t> SubParser::parseDirectDelimiter(const void *buf, size_t count)
{
    size_t prevSize = m_unparsedBuffer.size();
    std::optional<size_t> delimPos;
    m_delimiterFound = "";

    std::optional<size_t> bytesAppended = appendToUnparsedBuffer(buf,count);
    size_t bytesToDisplace = 0;

    // Error appending data...
    if (bytesAppended == std::nullopt)
        return std::nullopt;

    bytesToDisplace = bytesAppended.value();

    // Find the delimiter pos.
    delimPos = m_unparsedBuffer.find(m_parseDelimiter.c_str(), m_parseDelimiter.size());
    if (delimPos == std::nullopt)
    {
        // Not found, evaluate how much data I can give here...
        size_t bytesOfPossibleDelim = getLastBytesInCommon(m_parseDelimiter);
        switch (bytesOfPossibleDelim)
        {
        case 0:
            // delimiter not found at all, parse ALL direct...
            m_parsedBuffer.reference(&m_unparsedBuffer);
#ifdef DEBUG_PARSER
            //printf("Parsing direct delimiter (%llu, until size %llu): ", postParsedBuffer.size(), leftToParse); postParsedBuffer.print(); printf("\n"); fflush(stdout);
#endif
            setParseStatus(parse());
            m_unparsedBuffer.clear(); // Reset the container data for the next element.
            break;
        default:
            // bytesOfPossibleDelim maybe belongs to the delimiter, need more data to continue and release the buffer...
            // we are safe on unparsedBuffer.size()-bytesOfPossibleDelim
            m_parsedBuffer.reference(&m_unparsedBuffer, 0, m_unparsedBuffer.size() - bytesOfPossibleDelim);
            setParseStatus(parse());

            if (bytesOfPossibleDelim)
                m_unparsedBuffer.displace(m_unparsedBuffer.size() - bytesOfPossibleDelim); // Displace the parsed elements and leave the possible delimiter in the buffer...
            else
                m_unparsedBuffer.clear(); // Reset the container data for the next element.
            break;
        }

        return bytesToDisplace; // Move all data copied.
    }
    else
    {
        // DELIMITER Found...
        m_delimiterFound = m_parseDelimiter;

        if (delimPos.value() == 0)
        {
            m_unparsedBuffer.clear(); // found on 0... give nothing to give
            m_parsedBuffer.reference(&m_unparsedBuffer);
        }
        else
            m_parsedBuffer.reference(&m_unparsedBuffer, 0, delimPos.value());

        setParseStatus(parse());
        m_unparsedBuffer.clear(); // Reset the container data for the next element.

        return (delimPos.value() - prevSize) + m_parseDelimiter.size();
    }
}

size_t SubParser::getLastBytesInCommon(const std::string &boundary)
{
    size_t maxBoundary = std::min(m_unparsedBuffer.size(), boundary.size() - 1);
    for (size_t v = maxBoundary; v != 0; v--)
    {
        Mantids30::Memory::Containers::B_Ref ref = referenceLastBytes(v);
        char *toCmp = (static_cast<char *>(malloc(ref.size())));
        ref.copyOut(toCmp, ref.size());
        if (!memcmp(toCmp, boundary.c_str(), ref.size()))
        {
            free(toCmp);
            return v;
        }
        free(toCmp);
    }
    return 0;
}

Mantids30::Memory::Containers::B_Ref SubParser::referenceLastBytes(const size_t &bytes)
{
    Mantids30::Memory::Containers::B_Ref r;
    r.reference(&m_unparsedBuffer, m_unparsedBuffer.size() - bytes);
    return r;
}


std::string SubParser::getFoundDelimiter() const
{
    return m_delimiterFound;
}

void SubParser::setParseMultiDelimiter(const std::list<std::string> &value)
{
    m_parseMultiDelimiter = value;
}

size_t SubParser::getUnparsedDataSize()
{
    return m_unparsedBuffer.getSizeLeft();
}

void SubParser::setParseDataTargetSize(const size_t &value)
{
    m_unparsedBuffer.setMaxSize(value);
}

void SubParser::clear()
{
    m_unparsedBuffer.clear();
    m_parsedBuffer.clear();
}

const std::string &SubParser::getSubParserName() const
{
    return m_subParserName;
}

bool SubParser::isStreamEnded() const
{
    return m_streamEnded;
}

void SubParser::setParseMode(const ParseMode &value)
{
    if (value == PARSE_MODE_DIRECT)
        setParseDataTargetSize(std::numeric_limits<size_t>::max());
    m_parseMode = value;
}
