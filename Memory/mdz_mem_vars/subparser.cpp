#include "subparser.h"
#include "b_ref.h"



using namespace Mantids::Memory::Streams;


SubParser::SubParser()
{
    clientMode = true;
    upStream = nullptr;
    streamEnded = false;
    setParseStatus(PARSE_STAT_GET_MORE_DATA);
    parseMode = PARSE_MODE_DELIMITER;
    parseDelimiter = "\r\n";
}

SubParser::~SubParser()
{
}

void SubParser::initElemParser(Memory::Streams::StreamableObject *upstreamObj, bool clientMode)
{
    this->upStream = upstreamObj;
    this->clientMode = clientMode;
}

std::pair<bool,uint64_t> SubParser::writeIntoParser(const void *buf, size_t count)
{
    if (!count) streamEnded = true;
    switch (parseMode)
    {
    case PARSE_MODE_DELIMITER:

#ifdef DEBUG
        printf("Parse by delimiter %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, (char *)buf, count);
#endif
        return parseByDelimiter(buf,count);
    case PARSE_MODE_SIZE:
#ifdef DEBUG
        printf("Parse by size %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, (char *)buf, count);
#endif
        return parseBySize(buf,count);
    case PARSE_MODE_VALIDATOR:
#ifdef DEBUG
        printf("Parse by validator %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, (char *)buf, count);
#endif
        return parseByValidator(buf,count);
    case PARSE_MODE_DIRECT_DELIMITER:
#ifdef DEBUG
        printf("Parse by direct delimiter %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, (char *)buf, count);
#endif
        return parseDirectDelimiter(buf,count);
    case PARSE_MODE_CONNECTION_END:
#ifdef DEBUG
        printf("Parse by connection end %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, (char *)buf, count);
#endif
        return parseByConnectionEnd(buf,count);
    case PARSE_MODE_MULTIDELIMITER:
#ifdef DEBUG
        printf("Parse by multidelimiter %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, (char *)buf, count);
#endif
        return parseByMultiDelimiter(buf,count);
    case PARSE_MODE_DIRECT:
#ifdef DEBUG
        printf("Parse by direct %p\n", this); fflush(stdout);
        BIO_dump_fp (stdout, (char *)buf, count);
#endif
        return parseDirect(buf,count);
    default:
        break;
    }
    return std::make_pair(false,(uint64_t)0);
}

size_t SubParser::ParseValidator(Mantids::Memory::Containers::B_Base &bc)
{
    return std::numeric_limits<size_t>::max();
}

SubParser::ParseStatus SubParser::getParseStatus() const
{
    return parseStatus;
}

void SubParser::setParseStatus(const ParseStatus &value)
{
    parseStatus = value;
}

void SubParser::setParseDelimiter(const std::string &value)
{
    parseDelimiter = value;
}

Mantids::Memory::Containers::B_Base *SubParser::getParsedBuffer()
{
    return &parsedBuffer;
}

std::pair<bool,uint64_t> SubParser::parseByMultiDelimiter(const void *buf, size_t count)
{
    std::pair<bool,uint64_t> needlePos, bytesAppended = std::make_pair(true,0);
    uint64_t prevSize = unparsedBuffer.size(), bytesToDisplace = 0;
    if (!count)
    {
        streamEnded = true;
    }

    // stick to the unparsedBuffer object size.
    if ( count > unparsedBuffer.getSizeLeft() && unparsedBuffer.getSizeLeft() )
        count = unparsedBuffer.getSizeLeft();

    if (count && ((bytesAppended=unparsedBuffer.append(buf,count)).first==false || bytesAppended.second==0))
    {
        // size exceeded. don't continue with the streamparser, error.
        unparsedBuffer.clear();
        return std::make_pair(false,(uint64_t)0);
    }

    bytesToDisplace = bytesAppended.second;
    parsedBuffer.reference(&unparsedBuffer);

    if ( (needlePos = unparsedBuffer.find(parseMultiDelimiter, delimiterFound)).first!=false )
    {
        //std::cout << "MULTI DELIMITER FOUND AT ->" << needlePos.second << std::endl << std::flush;
        // needle found.
        parsedBuffer.reference(&unparsedBuffer,0,needlePos.second);

#ifdef DEBUG_PARSER
        printf("Parsing by multidelimiter: %s\n", postParsedBuffer.toString().c_str()); fflush(stdout);
#endif

        setParseStatus(parse());
        unparsedBuffer.clear();

        // Bytes to displace:
        bytesToDisplace = needlePos.second+delimiterFound.size()-prevSize;
    }
    else if (streamEnded)
    {
        parsedBuffer.reference(&unparsedBuffer);
        setParseStatus(parse());
        unparsedBuffer.clear();
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

    uint64_t prevSize = unparsedBuffer.size(), bytesToDisplace = 0;
    if (!count)
    {
        streamEnded = true;
    }

    // stick to the unparsedBuffer object size.
    if ( count > unparsedBuffer.getSizeLeft() && unparsedBuffer.getSizeLeft() )
        count = unparsedBuffer.getSizeLeft();

    if (count && ((bytesAppended=unparsedBuffer.append(buf,count)).second==0 || bytesAppended.first==false))
    {
#ifdef DEBUG
        printf("%p Parse by Delimiter size exceeded. don't continue with the streamparser, error....\n",this); fflush(stdout);
#endif
        // size exceeded. don't continue with the streamparser, error.
        unparsedBuffer.clear();
        return std::make_pair(false,(uint64_t)0);
    }

    bytesToDisplace = bytesAppended.second;
    parsedBuffer.reference(&unparsedBuffer);

#ifdef DEBUG
        auto x = unparsedBuffer.toString();
        printf("%p Searching delimiter...\n",this); fflush(stdout);
        BIO_dump_fp (stdout, (char *)parseDelimiter.c_str(), parseDelimiter.size());
        BIO_dump_fp (stdout, (char *)x.c_str(), x.size());
#endif

    if ( (needlePos = unparsedBuffer.find(parseDelimiter.c_str(),parseDelimiter.size())).first!=false )
    {
        // needle found.
        parsedBuffer.reference(&unparsedBuffer,0,needlePos.second);

#ifdef DEBUG_PARSER
        printf("Parsing by delimiter: %s\n", postParsedBuffer.toString().c_str()); fflush(stdout);
#endif
        setParseStatus(parse());
        unparsedBuffer.clear();

        // Bytes to displace:
        bytesToDisplace = (needlePos.second-prevSize)+parseDelimiter.size();
#ifdef DEBUG
        printf("%p Delimiter found and parsed -> displaced %lu bytes\n", this, bytesToDisplace); fflush(stdout);
#endif
    }
    else if (streamEnded)
    {
        parsedBuffer.reference(&unparsedBuffer);
#ifdef DEBUG
        printf("%p Delimiter not found and parsed, displaced %lu bytes\n", this, bytesToDisplace); fflush(stdout);
#endif
        setParseStatus(parse());
        unparsedBuffer.clear();
    }

#ifdef DEBUG
    printf("%p displacing %lu bytes\n", this, bytesToDisplace); fflush(stdout);
#endif

    return std::make_pair(true,bytesToDisplace);
}

std::pair<bool,uint64_t> SubParser::parseBySize(const void *buf, size_t count)
{
    if (!count)
    {
        // Abort current data.
        streamEnded = true;
        setParseStatus(PARSE_STAT_GOTO_NEXT_SUBPARSER);
        unparsedBuffer.clear(); // Destroy the container data.
        return std::make_pair(true,0);
    }

    count = count>unparsedBuffer.getSizeLeft()?unparsedBuffer.getSizeLeft():count;
    std::pair<bool,uint64_t> appendResult = unparsedBuffer.append(buf,count);

    if (!appendResult.first)
        return appendResult;

    // Ready...
    if (unparsedBuffer.getSizeLeft()==0)
    {
        parsedBuffer.reference(&unparsedBuffer);
#ifdef DEBUG_PARSER
        printf("Parsing by size: %s\n", postParsedBuffer.toString().c_str()); fflush(stdout);
#endif
        setParseStatus(parse());
        unparsedBuffer.clear(); // Destroy the container data.
    }

    return std::make_pair(true,appendResult.second);
}

std::pair<bool,uint64_t> SubParser::parseByValidator(const void *buf, size_t count)
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
    return std::make_pair(false,(uint64_t)0);
}

std::pair<bool,uint64_t> SubParser::parseByConnectionEnd(const void *buf, size_t count)
{
    // stick to the unparsedBuffer object size.
    if ( count > unparsedBuffer.getSizeLeft() && unparsedBuffer.getSizeLeft() )
        count = unparsedBuffer.getSizeLeft();

    std::pair<bool,uint64_t> appendedBytes = unparsedBuffer.append(buf,count);

    if (!appendedBytes.first)
        return std::make_pair(false,(uint64_t)0);
    if (appendedBytes.second!=count)
        return std::make_pair(false,appendedBytes.second); // max reached.

    if (!count)
    {
        parsedBuffer.reference(&unparsedBuffer);
#ifdef DEBUG_PARSER
        printf("Parsing by connection end: %s\n", postParsedBuffer.toString().c_str()); fflush(stdout);
#endif
        setParseStatus(parse()); // analyze on connection end.
        parsedBuffer.clear();
        return std::make_pair(true,0);
    }
    return std::make_pair(true,count);
}

std::pair<bool,uint64_t> SubParser::parseDirect(const void *buf, size_t count)
{
    std::pair<bool,uint64_t> copiedDirect;

    // TODO: check.
    // stick to the unparsedBuffer object size.
    if ( count > unparsedBuffer.getSizeLeft() && unparsedBuffer.getSizeLeft() )
        count = unparsedBuffer.getSizeLeft();

    // TODO: termination?

    // Bad copy...
    if ((copiedDirect = unparsedBuffer.append(buf, count)).first == false)
        return std::make_pair(false,(uint64_t)0);

    // Parse it...
    parsedBuffer.reference(&unparsedBuffer);
#ifdef DEBUG_PARSER
    printf("Parsing direct (%llu, until size %llu): %s\n", postParsedBuffer.size(), leftToParse, postParsedBuffer.toString().c_str()); fflush(stdout);
#endif
    setParseStatus(parse());

    // All the unparsed buffer was consumed by parsed buffer to the next parser...
    auto curBufSize = unparsedBuffer.size();
    unparsedBuffer.clear(); // Reset the container data for the next element.
    unparsedBuffer.reduceMaxSizeBy( curBufSize );

    return copiedDirect;
}


std::pair<bool,uint64_t> SubParser::parseDirectDelimiter(const void *buf, size_t count)
{
    uint64_t prevSize=0;
    std::pair<bool,uint64_t> delimPos;
    std::pair<bool,uint64_t> copiedDirect;
    delimiterFound = "";

    // stick to the unparsedBuffer object size.
    if ( count > unparsedBuffer.getSizeLeft() && unparsedBuffer.getSizeLeft() )
        count = unparsedBuffer.getSizeLeft();

    if (count == 0)
        return std::make_pair(false,(uint64_t)0); // Can't handle more data...

    // Get the previous size.
    prevSize = unparsedBuffer.size();       

    // Append the current data
    if ((copiedDirect = unparsedBuffer.append(buf,count)).first==false)
        return std::make_pair(false,(uint64_t)0);

    // Find the delimiter pos.
    if ((delimPos = unparsedBuffer.find(parseDelimiter.c_str(),parseDelimiter.size())).first==false)
    {
        // Not found, evaluate how much data I can give here...
        uint64_t bytesOfPossibleDelim = getLastBytesInCommon(parseDelimiter);
        switch (bytesOfPossibleDelim)
        {
        case 0:
            // delimiter not found at all, parse ALL direct...
            parsedBuffer.reference(&unparsedBuffer);
#ifdef DEBUG
            //printf("Parsing direct delimiter (%llu, until size %llu): ", postParsedBuffer.size(), leftToParse); postParsedBuffer.print(); printf("\n"); fflush(stdout);
#endif
            setParseStatus(parse());
            unparsedBuffer.clear(); // Reset the container data for the next element.
            break;
        default:
            // bytesOfPossibleDelim maybe belongs to the delimiter, need more data to continue and release the buffer...
            // we are safe on unparsedBuffer.size()-bytesOfPossibleDelim
            parsedBuffer.reference(&unparsedBuffer, 0,unparsedBuffer.size()-bytesOfPossibleDelim);
            setParseStatus(parse());

            if ( bytesOfPossibleDelim )
                unparsedBuffer.displace( unparsedBuffer.size()-bytesOfPossibleDelim ); // Displace the parsed elements and leave the possible delimiter in the buffer...
            else
                unparsedBuffer.clear(); // Reset the container data for the next element.
            break;
        }

        return std::make_pair(true,copiedDirect.second); // Move all data copied.
    }
    else
    {
        // DELIMITER Found...
        delimiterFound = parseDelimiter;

        if (delimPos.second==0)
        {
            unparsedBuffer.clear(); // found on 0... give nothing to give
            parsedBuffer.reference(&unparsedBuffer);
        }
        else
            parsedBuffer.reference(&unparsedBuffer,0,delimPos.second);

        setParseStatus(parse());
        unparsedBuffer.clear(); // Reset the container data for the next element.

        return std::make_pair(true,(delimPos.second-prevSize)+parseDelimiter.size());
    }
}

uint64_t SubParser::getLastBytesInCommon(const std::string &boundary)
{
    size_t maxBoundary = unparsedBuffer.size()>(boundary.size()-1)?(boundary.size()-1) : unparsedBuffer.size();
    for (size_t v=maxBoundary; v!=0; v--)
    {
        Mantids::Memory::Containers::B_Ref ref = referenceLastBytes(v);
        char * toCmp = ((char *)malloc(ref.size()));
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

Mantids::Memory::Containers::B_Ref SubParser::referenceLastBytes(const size_t &bytes)
{
    Mantids::Memory::Containers::B_Ref r;
    r.reference(&unparsedBuffer, unparsedBuffer.size()-bytes);
    return r;
}

std::string SubParser::getDelimiterFound() const
{
    return delimiterFound;
}

void SubParser::setParseMultiDelimiter(const std::list<std::string> &value)
{
    parseMultiDelimiter = value;
}

uint64_t SubParser::getLeftToparse() const
{
    return unparsedBuffer.getSizeLeft();
}

void SubParser::setParseDataTargetSize(const uint64_t &value)
{
    unparsedBuffer.setMaxSize(value);
}

void SubParser::clear()
{
    unparsedBuffer.clear();
    parsedBuffer.clear();
}

const std::string &SubParser::getSubParserName() const
{
    return subParserName;
}

bool SubParser::isStreamEnded() const
{
    return streamEnded;
}

void SubParser::setParseMode(const ParseMode &value)
{
    if (value == PARSE_MODE_DIRECT) setParseDataTargetSize(std::numeric_limits<uint64_t>::max());
    parseMode = value;
}
