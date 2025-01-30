#include "common_urlvars.h"

#include "streamencoder_url.h"
#include <boost/algorithm/string.hpp>
#include <memory>

using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols::HTTP;
using namespace Mantids30::Network::Protocols::HTTP::Common;
using namespace Mantids30;

URLVars::URLVars(std::shared_ptr<Memory::Streams::StreamableObject> value) : Memory::Streams::Parser(value,false)
{
    initSubParser(&_urlVarParser);

    // TODO: virtual method during constructor...
    m_initialized = initProtocol();
    currentStat = URLV_STAT_WAITING_NAME;
    _urlVarParser.setVarType(true);

    setMaxVarNameSize(4096);
    setMaxVarContentSize(4096);

    m_currentParser = &_urlVarParser;
}

URLVars::~URLVars()
{
    //for (auto & i : vars) delete i.second;
}

bool URLVars::isEmpty()
{
    return vars.empty();
}

//(Memory::Containers::B_Chunks *)

bool URLVars::streamTo(std::shared_ptr<Memory::Streams::StreamableObject> out, Memory::Streams::StreamableObject::Status &wrsStat)
{
    Memory::Streams::StreamableObject::Status cur;
    bool firstVar = true;
    for (auto & i : vars)
    {
        if (firstVar) firstVar=false;
        else
        {
            if (!(cur+=out->writeString("&", wrsStat)).succeed)
                return false;
        }

        Memory::Containers::B_Chunks varName;
        varName.append(i.first.c_str(), i.first.size());

        std::shared_ptr<Memory::Streams::Encoders::URL> varNameEncoder = std::make_shared<Memory::Streams::Encoders::URL>(out);
        //bytesWritten+=varNameEncoder.getFinalBytesWritten();
        if (!(cur+=varName.streamTo(varNameEncoder, wrsStat)).succeed)
        {
            out->writeEOF(false);
            return false;
        }

        if ((i.second)->size())
        {
            if (!(cur+=out->writeString("=",wrsStat)).succeed)
                return false;

            std::shared_ptr<Memory::Streams::Encoders::URL> varNameEncoder2 = std::make_shared<Memory::Streams::Encoders::URL>(out);
            //writtenBytes+=varNameEncoder2.getFinalBytesWritten();
            if (!(i.second)->streamTo(varNameEncoder2,wrsStat))
            {
                out->writeEOF(false);
                return false;
            }
        }
    }
    out->writeEOF(true);
    return true;
}

uint32_t URLVars::varCount(const std::string &varName)
{
    uint32_t i=0;
    auto range = vars.equal_range(boost::to_upper_copy(varName));
    for (auto iterator = range.first; iterator != range.second;) i++;
    return i;
}

std::shared_ptr<Memory::Streams::StreamableObject> URLVars::getValue(const std::string &varName)
{
    auto range = vars.equal_range(boost::to_upper_copy(varName));

    for (auto iterator = range.first; iterator != range.second;)
        return iterator->second;

    return nullptr;
}

std::list<std::shared_ptr<Memory::Streams::StreamableObject> > URLVars::getValues(const std::string &varName)
{
    std::list<std::shared_ptr<Memory::Streams::StreamableObject> > r;
    auto range = vars.equal_range(boost::to_upper_copy(varName));

    for (auto iterator = range.first; iterator != range.second;)
        r.push_back(iterator->second);
    return r;
}

std::set<std::string> URLVars::getKeysList()
{
    std::set<std::string> r;
    for ( const auto & i : vars ) r.insert(i.first);
    return r;
}

bool URLVars::initProtocol()
{
    return true;
}

void URLVars::endProtocol()
{
}

bool URLVars::changeToNextParser()
{
    switch(currentStat)
    {
    case URLV_STAT_WAITING_NAME:
    {
        currentVarName = _urlVarParser.flushRetrievedContentAsString();
        if (_urlVarParser.getDelimiterFound() == "&" || _urlVarParser.isStreamEnded())
        {
            // AMP / END:
            addVar(currentVarName, _urlVarParser.flushRetrievedContentAsBC());
        }
        else
        {
            // EQUAL:
            currentStat = URLV_STAT_WAITING_CONTENT;
            _urlVarParser.setVarType(false);
            _urlVarParser.setMaxObjectSize(maxVarContentSize);
        }
    }break;
    case URLV_STAT_WAITING_CONTENT:
    {
        addVar(currentVarName, _urlVarParser.flushRetrievedContentAsBC());
        currentStat = URLV_STAT_WAITING_NAME;
        _urlVarParser.setVarType(true);
        _urlVarParser.setMaxObjectSize(maxVarNameSize);
    }break;
    default:
        break;
    }

    return true;
}

void URLVars::addVar(const std::string &varName, std::shared_ptr<Memory::Containers::B_Chunks> data)
{
    //vars.insert(std::pair<std::string,Memory::Containers::B_Chunks*>(currentVarName, _urlVarParser.flushRetrievedContentAsBC()));
    if (!varName.empty())
        vars.insert(std::pair<std::string, std::shared_ptr<Memory::Containers::B_Chunks>>(boost::to_upper_copy(varName), data));
/*    else
        delete data;*/
}


void URLVars::iSetMaxVarContentSize()
{
    if (currentStat == URLV_STAT_WAITING_CONTENT) _urlVarParser.setMaxObjectSize(maxVarContentSize);
}

void URLVars::iSetMaxVarNameSize()
{
    if (currentStat == URLV_STAT_WAITING_NAME) _urlVarParser.setMaxObjectSize(maxVarNameSize);
}
