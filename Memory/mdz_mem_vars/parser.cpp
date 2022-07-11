#include "parser.h"
#include "b_ref.h"

using namespace Mantids::Memory::Streams;

Parser::Parser(Memory::Streams::StreamableObject *value, bool clientMode)
{
    this->clientMode = clientMode;
    currentParser = nullptr;
    maxTTL = 4096;
    initialized = false;
    this->streamableObject = value;
}

Parser::~Parser()
{
}

StreamableObject::Status Parser::parseObject(Parser::ErrorMSG *err)
{
    bool ret;
    Status upd;

    *err = PARSING_SUCCEED;
    initialized = initProtocol();
    if (initialized)
    {
        if (!(ret=streamableObject->streamTo(this,upd)) || !upd.succeed)
        {
            upd.succeed=false;
            *err=getFailedWriteState()!=0?PARSING_ERR_READ:PARSING_ERR_PARSE;
        }
        endProtocol();
        return upd;
    }
    else
        upd.succeed=false;

    *err = PARSING_ERR_INIT;
    return upd;
}

bool Parser::streamTo(Memory::Streams::StreamableObject *out, Status &wrsStat)
{
    return false;
}

StreamableObject::Status Parser::write(const void *buf, const size_t &count, Status &wrStat)
{
    Status ret;
    // Parse this data...
    size_t ttl = 0;
    bool finished = false;

    std::pair<bool, uint64_t> r = parseData(buf,count, &ttl, &finished);
    if (finished) ret.finish = wrStat.finish = true;

    if (r.first==false)
    {
        wrStat.succeed = ret.succeed = setFailedWriteState();
    }
    else
    {
        ret+=r.second;
        wrStat+=ret;
    }

    return ret;
}

void Parser::writeEOF(bool)
{
    size_t ttl = 0;
    bool finished = false;
    parseData("",0, &ttl,&finished);
}

std::pair<bool, uint64_t> Parser::parseData(const void *buf, size_t count, size_t *ttl, bool *finished)
{
    // TODO: reset TTL?
    if (*ttl>maxTTL)
        return std::make_pair(false,(uint64_t)0); // TTL Reached...
//    std::cout << "Reaching TTL " << *ttl << std::endl << std::flush;
    (*ttl)++;
    std::pair<bool, uint64_t> writtenBytes;

    if (currentParser!=nullptr)
    {
        // Default state: get more data...
        currentParser->setParseStatus(SubParser::PARSE_STAT_GET_MORE_DATA);
        // Here, the parser should call the sub stream parser parse function and set the new status.
        if ((writtenBytes=currentParser->writeIntoParser(buf,count)).first==false)
            return std::make_pair(false,(uint64_t)0);
        // TODO: what if error? how to tell the parser that it should analize the connection up to there (without correctness).
        switch (currentParser->getParseStatus())
        {
        case SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER:
        {
         //   std::cout << std::asctime(std::localtime(&result)) << "PARSE_STAT_GOTO_NEXT_SUBPARSER" << std::endl << std::flush;
            // Check if there is next parser...
            if (!changeToNextParser())
                return std::make_pair(false,(uint64_t)0);
            // If the parser is changed to nullptr, then the connection is ended (-2).
            // Parsed OK :)... Pass to the next stage
            if (currentParser==nullptr) *finished = true;
            if (currentParser==nullptr || writtenBytes.second == count)
                return writtenBytes;
        } break;
        case SubParser::PARSE_STAT_GET_MORE_DATA:
        {
        //    std::cout << std::asctime(std::localtime(&result)) << "PARSE_STAT_GET_MORE_DATA" << std::endl << std::flush;
            // More data required... (TODO: check this)
            if (writtenBytes.second == count)
                return writtenBytes;
        } break;
            // Bad parsing... end here.
        case SubParser::PARSE_STAT_ERROR:
         //   std::cout << std::asctime(std::localtime(&result)) << "PARSE_STAT_ERROR" << std::endl << std::flush;
            return std::make_pair(false,(uint64_t)0);
            // Unknown parser...
        }
    }
    // TODO: what if writtenBytes == 0?

    // Data left, process it.
    if (writtenBytes.second!=count)
    {
        buf=((const char *)buf)+writtenBytes.second;
        count-=writtenBytes.second;

        // Data left to process..
        std::pair<bool, uint64_t> x;
        if ((x=parseData(buf,count, ttl, finished)).first==false)
            return x;

        return std::make_pair(true,x.second+writtenBytes.second);
    }
    else return writtenBytes;
}

void Parser::initSubParser(SubParser *subparser)
{
    subparser->initElemParser(streamableObject?streamableObject:this,clientMode);
}

void Parser::setMaxTTL(const size_t &value)
{
    maxTTL = value;
}

void Parser::setStreamable(Memory::Streams::StreamableObject *value)
{
    streamableObject = value;
}

