#include "subparser.h"
#include "b_ref.h"



using namespace Mantids30::Memory::Streams;


SubParser::SubParser()
{
    m_clientMode = true;
    m_upStream = nullptr;
    m_streamEnded = false;
    setParseStatus(PARSE_STAT_GET_MORE_DATA);
    m_parseMode = PARSE_MODE_DELIMITER;
    m_parseDelimiter = "\r\n";
}


void SubParser::initElemParser(std::shared_ptr<Memory::Streams::StreamableObject> upstreamObj, bool clientMode)
{
    this->m_upStream = upstreamObj;
    this->m_clientMode = clientMode;
}

std::pair<bool,uint64_t> SubParser::writeIntoParser(const void *buf, size_t count)
{
    if (!count) m_streamEnded = true;
    switch (m_parseMode)
    {
    case PARSE_MODE_DELIMITER:

#ifdef DEBUG_PARSER
        printf("Parse by delimiter %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, static_cast<char *>(buf), count);
#endif
        return parseByDelimiter(buf,count);
    case PARSE_MODE_SIZE:
#ifdef DEBUG_PARSER
        printf("Parse by size %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, static_cast<char *>(buf), count);
#endif
        return parseBySize(buf,count);
    case PARSE_MODE_VALIDATOR:
#ifdef DEBUG_PARSER
        printf("Parse by validator %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, static_cast<char *>(buf), count);
#endif
        return parseByValidator(buf,count);
    case PARSE_MODE_DIRECT_DELIMITER:
#ifdef DEBUG_PARSER
        printf("Parse by direct delimiter %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, static_cast<char *>(buf), count);
#endif
        return parseDirectDelimiter(buf,count);
    case PARSE_MODE_CONNECTION_END:
#ifdef DEBUG_PARSER
        printf("Parse by connection end %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, static_cast<char *>(buf), count);
#endif
        return parseByConnectionEnd(buf,count);
    case PARSE_MODE_MULTIDELIMITER:
#ifdef DEBUG_PARSER
        printf("Parse by multidelimiter %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, static_cast<char *>(buf), count);
#endif
        return parseByMultiDelimiter(buf,count);
    case PARSE_MODE_DIRECT:
#ifdef DEBUG_PARSER
        printf("Parse by direct %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, static_cast<char *>(buf), count);
#endif
        return parseDirect(buf,count);
    default:
        break;
    }
    return std::make_pair(false,static_cast<uint64_t>(0));
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

std::pair<bool,uint64_t> SubParser::parseByMultiDelimiter(const void *buf, size_t count)
{
    std::pair<bool,uint64_t> needlePos, bytesAppended = std::make_pair(true,0);
    uint64_t prevSize = m_unparsedBuffer.size(), bytesToDisplace = 0;
    if (!count)
    {
        m_streamEnded = true;
    }

    // stick to the unparsedBuffer object size.
    if ( count > m_unparsedBuffer.getSizeLeft() && m_unparsedBuffer.getSizeLeft() )
        count = m_unparsedBuffer.getSizeLeft();

    if (count && ((bytesAppended=m_unparsedBuffer.append(buf,count)).first==false || bytesAppended.second==0))
    {
        // size exceeded. don't continue with the streamparser, error.
        m_unparsedBuffer.clear();
        return std::make_pair(false,static_cast<uint64_t>(0));
    }

    bytesToDisplace = bytesAppended.second;
    m_parsedBuffer.reference(&m_unparsedBuffer);

    if ( (needlePos = m_unparsedBuffer.find(m_parseMultiDelimiter, m_delimiterFound)).first!=false )
    {
        //std::cout << "MULTI DELIMITER FOUND AT ->" << needlePos.second << std::endl << std::flush;
        // needle found.
        m_parsedBuffer.reference(&m_unparsedBuffer,0,needlePos.second);

#ifdef DEBUG_PARSER
        printf("Parsing by multidelimiter: %s\n", m_parsedBuffer.toString().c_str()); fflush(stdout);
#endif

        setParseStatus(parse());
        m_unparsedBuffer.clear();

        // Bytes to displace:
        bytesToDisplace = needlePos.second+m_delimiterFound.size()-prevSize;
    }
    else if (m_streamEnded)
    {
        m_parsedBuffer.reference(&m_unparsedBuffer);
        setParseStatus(parse());
        m_unparsedBuffer.clear();
    }
    else
    {
        //std::cout << "DELIMITER NOT FOUND HERE." << std::endl << std::flush;
    }

    return std::make_pair(true,bytesToDisplace);
}

std::pair<bool,uint64_t> SubParser::parseByDelimiter(const void *buf, size_t count)
{
    std::pair<bool,uint64_t> needlePos, bytesAppended = std::make_pair(true,0);

    uint64_t prevSize = m_unparsedBuffer.size(), bytesToDisplace = 0;
    if (!count)
    {
        m_streamEnded = true;
    }

    // stick to the unparsedBuffer object size.
    if ( count > m_unparsedBuffer.getSizeLeft() && m_unparsedBuffer.getSizeLeft() )
    {
        count = m_unparsedBuffer.getSizeLeft();
    }

    if (count && ((bytesAppended=m_unparsedBuffer.append(buf,count)).second==0 || bytesAppended.first==false))
    {
#ifdef DEBUG_PARSER
        printf("%p Parse by Delimiter size exceeded. don't continue with the streamparser, error....\n",this); fflush(stdout);
#endif
        // size exceeded. don't continue with the streamparser, error.
        m_unparsedBuffer.clear();
        return std::make_pair(false,static_cast<uint64_t>(0));
    }

    bytesToDisplace = bytesAppended.second;
    m_parsedBuffer.reference(&m_unparsedBuffer);

#ifdef DEBUG_PARSER
        auto x = m_unparsedBuffer.toString();
        printf("%p Searching delimiter...\n",this); fflush(stdout);
        BIO_dump_fp (stdout, (char *)parseDelimiter.c_str(), m_parseDelimiter.size());
        BIO_dump_fp (stdout, (char *)x.c_str(), x.size());
#endif

    if ( (needlePos = m_unparsedBuffer.find(m_parseDelimiter.c_str(),m_parseDelimiter.size())).first!=false )
    {
        // needle found.
        m_parsedBuffer.reference(&m_unparsedBuffer,0,needlePos.second);

#ifdef DEBUG_PARSER
        printf("Parsing by delimiter: %s\n", m_parsedBuffer.toString().c_str()); fflush(stdout);
#endif
        setParseStatus(parse());
        m_unparsedBuffer.clear();

        // Bytes to displace:
        bytesToDisplace = (needlePos.second-prevSize)+m_parseDelimiter.size();
#ifdef DEBUG_PARSER
        printf("%p Delimiter found and parsed -> displaced %lu bytes\n", this, bytesToDisplace); fflush(stdout);
#endif
    }
    else if (m_streamEnded)
    {
        m_parsedBuffer.reference(&m_unparsedBuffer);
#ifdef DEBUG_PARSER
        printf("%p Delimiter not found and parsed, displaced %lu bytes\n", this, bytesToDisplace); fflush(stdout);
#endif
        setParseStatus(parse());
        m_unparsedBuffer.clear();
    }

#ifdef DEBUG_PARSER
    printf("%p displacing %lu bytes\n", this, bytesToDisplace); fflush(stdout);
#endif

    return std::make_pair(true,bytesToDisplace);
}

std::pair<bool,uint64_t> SubParser::parseBySize(const void *buf, size_t count)
{
    if (!count)
    {
        // Abort current data.
        m_streamEnded = true;
        setParseStatus(PARSE_STAT_GOTO_NEXT_SUBPARSER);
        m_unparsedBuffer.clear(); // Destroy the container data.
        return std::make_pair(true,0);
    }

    count = count>m_unparsedBuffer.getSizeLeft()?m_unparsedBuffer.getSizeLeft():count;
    std::pair<bool,uint64_t> appendResult = m_unparsedBuffer.append(buf,count);

    if (!appendResult.first)
        return appendResult;

    // Ready...
    if (m_unparsedBuffer.getSizeLeft()==0)
    {
        m_parsedBuffer.reference(&m_unparsedBuffer);
#ifdef DEBUG_PARSER
        printf("Parsing by size: %s\n", m_parsedBuffer.toString().c_str()); fflush(stdout);
#endif
        setParseStatus(parse());
        m_unparsedBuffer.clear(); // Destroy the container data.
    }

    return std::make_pair(true,appendResult.second);
}

std::pair<bool,uint64_t> SubParser::parseByValidator(const void *, size_t )
{
    // TODO: parseByValidator
    /*B_MEM bc;

    size_t origSize = postParsedBuffer.getContainerSize();

    bc.setMaxSize(postParsedBuffer.getMaxSize());
    postParsedBuffer.appendTo(bc);
    if (bc.append(buf,count)!=count) return -1;
    size_t validatedBytes = ParseValidator(bc);
    if (validatedBytes == -1)
    {
        // NOT FOUND:
        if (postParsedBuffer.append(buf,count)!=count) return -1;
        return count;
    }

    size_t bytesLeft = bc.getContainerSize()-validatedBytes;
    */
    return std::make_pair(false,static_cast<uint64_t>(0));
}

std::pair<bool,uint64_t> SubParser::parseByConnectionEnd(const void *buf, size_t count)
{
    // stick to the unparsedBuffer object size.
    if ( count > m_unparsedBuffer.getSizeLeft() && m_unparsedBuffer.getSizeLeft() )
        count = m_unparsedBuffer.getSizeLeft();

    std::pair<bool,uint64_t> appendedBytes = m_unparsedBuffer.append(buf,count);

    if (!appendedBytes.first)
        return std::make_pair(false,static_cast<uint64_t>(0));
    if (appendedBytes.second!=count)
        return std::make_pair(false,appendedBytes.second); // max reached.

    if (!count)
    {
        m_parsedBuffer.reference(&m_unparsedBuffer);
#ifdef DEBUG_PARSER
        printf("Parsing by connection end: %s\n", m_parsedBuffer.toString().c_str()); fflush(stdout);
#endif
        setParseStatus(parse()); // analyze on connection end.
        m_parsedBuffer.clear();
        return std::make_pair(true,0);
    }
    return std::make_pair(true,count);
}

std::pair<bool,uint64_t> SubParser::parseDirect(const void *buf, size_t count)
{
    std::pair<bool,uint64_t> copiedDirect;

    // TODO: check.
    // stick to the unparsedBuffer object size.
    if ( count > m_unparsedBuffer.getSizeLeft() && m_unparsedBuffer.getSizeLeft() )
        count = m_unparsedBuffer.getSizeLeft();

    // TODO: termination?

    // Bad copy...
    if ((copiedDirect = m_unparsedBuffer.append(buf, count)).first == false)
        return std::make_pair(false,static_cast<uint64_t>(0));

    // Parse it...
    m_parsedBuffer.reference(&m_unparsedBuffer);
#ifdef DEBUG_PARSER
    printf("Parsing direct (%lu, until size %lu): %s\n", m_parsedBuffer.size(), static_cast<uint64_t>(0), m_parsedBuffer.toString().c_str()); fflush(stdout);
#endif
    setParseStatus(parse());

    // All the unparsed buffer was consumed by parsed buffer to the next parser...
    auto curBufSize = m_unparsedBuffer.size();
    m_unparsedBuffer.clear(); // Reset the container data for the next element.
    m_unparsedBuffer.reduceMaxSizeBy( curBufSize );

    return copiedDirect;
}


std::pair<bool,uint64_t> SubParser::parseDirectDelimiter(const void *buf, size_t count)
{
    uint64_t prevSize=0;
    std::pair<bool,uint64_t> delimPos;
    std::pair<bool,uint64_t> copiedDirect;
    m_delimiterFound = "";

    // stick to the unparsedBuffer object size.
    if ( count > m_unparsedBuffer.getSizeLeft() && m_unparsedBuffer.getSizeLeft() )
        count = m_unparsedBuffer.getSizeLeft();

    if (count == 0)
        return std::make_pair(false,static_cast<uint64_t>(0)); // Can't handle more data...

    // Get the previous size.
    prevSize = m_unparsedBuffer.size();       

    // Append the current data
    if ((copiedDirect = m_unparsedBuffer.append(buf,count)).first==false)
        return std::make_pair(false,static_cast<uint64_t>(0));

    // Find the delimiter pos.
    if ((delimPos = m_unparsedBuffer.find(m_parseDelimiter.c_str(),m_parseDelimiter.size())).first==false)
    {
        // Not found, evaluate how much data I can give here...
        uint64_t bytesOfPossibleDelim = getLastBytesInCommon(m_parseDelimiter);
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
            m_parsedBuffer.reference(&m_unparsedBuffer,0, m_unparsedBuffer.size()-bytesOfPossibleDelim);
            setParseStatus(parse());

            if ( bytesOfPossibleDelim )
                m_unparsedBuffer.displace( m_unparsedBuffer.size()-bytesOfPossibleDelim ); // Displace the parsed elements and leave the possible delimiter in the buffer...
            else
                m_unparsedBuffer.clear(); // Reset the container data for the next element.
            break;
        }

        return std::make_pair(true,copiedDirect.second); // Move all data copied.
    }
    else
    {
        // DELIMITER Found...
        m_delimiterFound = m_parseDelimiter;

        if (delimPos.second==0)
        {
            m_unparsedBuffer.clear(); // found on 0... give nothing to give
            m_parsedBuffer.reference(&m_unparsedBuffer);
        }
        else
            m_parsedBuffer.reference(&m_unparsedBuffer,0,delimPos.second);

        setParseStatus(parse());
        m_unparsedBuffer.clear(); // Reset the container data for the next element.

        return std::make_pair(true,(delimPos.second-prevSize)+m_parseDelimiter.size());
    }
}

uint64_t SubParser::getLastBytesInCommon(const std::string &boundary)
{
    size_t maxBoundary = m_unparsedBuffer.size()>(boundary.size()-1)?(boundary.size()-1) : m_unparsedBuffer.size();
    for (size_t v=maxBoundary; v!=0; v--)
    {
        Mantids30::Memory::Containers::B_Ref ref = referenceLastBytes(v);
        char * toCmp = (static_cast<char *>(malloc(ref.size())));
        ref.copyOut(toCmp,ref.size());
        if (!memcmp(toCmp,boundary.c_str(),ref.size()))
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
    r.reference(&m_unparsedBuffer, m_unparsedBuffer.size()-bytes);
    return r;
}

std::string SubParser::getDelimiterFound() const
{
    return m_delimiterFound;
}

void SubParser::setParseMultiDelimiter(const std::list<std::string> &value)
{
    m_parseMultiDelimiter = value;
}

uint64_t SubParser::getLeftToparse() const
{
    return m_unparsedBuffer.getSizeLeft();
}

void SubParser::setParseDataTargetSize(const uint64_t &value)
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
    if (value == PARSE_MODE_DIRECT) setParseDataTargetSize(std::numeric_limits<uint64_t>::max());
    m_parseMode = value;
}
