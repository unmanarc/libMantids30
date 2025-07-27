#include "common_urlvars.h"

#include "Mantids30/Memory/streamablenull.h"
#include "streamencoder_url.h"
#include <boost/algorithm/string.hpp>
#include <memory>

using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols;

using namespace Mantids30;

HTTP::URLVars::URLVars(std::shared_ptr<Memory::Streams::StreamableObject> value) : Memory::Streams::Parser(value,false)
{
    m_urlVarParser.setVarType(true);

    setMaxVarNameSize(4096);
    setMaxVarContentSize(4096);

    m_currentParser = &m_urlVarParser;
}

std::shared_ptr<HTTP::URLVars> HTTP::URLVars::create(
    std::shared_ptr<StreamableObject> value)
{
    auto x = std::shared_ptr<URLVars>(new URLVars(value));
    x->initSubParser(&x->m_urlVarParser);
    x->m_initialized = x->initProtocol();
    return x;
}

bool HTTP::URLVars::isEmpty()
{
    return m_vars.empty();
}

bool HTTP::URLVars::streamTo(Memory::Streams::StreamableObject * out)
{
    bool firstVar = true;
    for (auto & i : m_vars)
    {
        if (firstVar)
            firstVar=false;
        else
        {
            out->writeString("&");
            if (!out->writeStatus.succeed)
                return false;
        }

        Memory::Containers::B_Chunks varName;
        varName.append(i.first.c_str(), i.first.size());

        Memory::Streams::Encoders::URL varNameEncoder;

        varNameEncoder.transform(&varName,out);

        if (!out->writeStatus.succeed)
            return false;

        if ((i.second)->size())
        {
            out->writeString("=");

            if (!out->writeStatus.succeed)
                return false;

            Memory::Streams::Encoders::URL varNameEncoder2;
            varNameEncoder2.transform(i.second.get(),out);
            if (!out->writeStatus.succeed)
                return false;
        }
    }

    return true;
    //return out->writeEOF();
}

uint32_t HTTP::URLVars::varCount(const std::string &varName)
{
    uint32_t i=0;
    auto range = m_vars.equal_range(boost::to_upper_copy(varName));
    for (auto iterator = range.first; iterator != range.second;) i++;
    return i;
}

std::shared_ptr<Memory::Streams::StreamableObject> HTTP::URLVars::getValue(const std::string &varName)
{
    auto range = m_vars.equal_range(boost::to_upper_copy(varName));

    for (auto iterator = range.first; iterator != range.second;)
        return iterator->second;

    return nullptr;
}

std::list<std::shared_ptr<Memory::Streams::StreamableObject> > HTTP::URLVars::getValues(const std::string &varName)
{
    std::list<std::shared_ptr<Memory::Streams::StreamableObject> > r;
    auto range = m_vars.equal_range(boost::to_upper_copy(varName));

    for (auto iterator = range.first; iterator != range.second;)
        r.push_back(iterator->second);
    return r;
}

std::set<std::string> HTTP::URLVars::getKeysList()
{
    std::set<std::string> r;
    for ( const auto & i : m_vars ) r.insert(i.first);
    return r;
}

bool HTTP::URLVars::initProtocol()
{
    return true;
}

void HTTP::URLVars::endProtocol()
{
}

bool HTTP::URLVars::changeToNextParser()
{
    switch(m_currentStat)
    {
    case URLV_STAT_WAITING_NAME:
    {
        m_currentVarName = m_urlVarParser.flushRetrievedContentAsString();
        if (m_urlVarParser.getFoundDelimiter() == "&" || m_urlVarParser.isStreamEnded())
        {
            // AMP / END:
            addVar(m_currentVarName, m_urlVarParser.flushRetrievedContentAsBC());
        }
        else
        {
            // EQUAL:
            m_currentStat = URLV_STAT_WAITING_CONTENT;
            m_urlVarParser.setVarType(false);
            m_urlVarParser.setMaxObjectSize(m_maxVarContentSize);
        }
    }break;
    case URLV_STAT_WAITING_CONTENT:
    {
        addVar(m_currentVarName, m_urlVarParser.flushRetrievedContentAsBC());
        m_currentStat = URLV_STAT_WAITING_NAME;
        m_urlVarParser.setVarType(true);
        m_urlVarParser.setMaxObjectSize(m_maxVarNameSize);
    }break;
    default:
        break;
    }

    return true;
}

bool HTTP::URLVars::addVar(
    const std::string &varName, std::shared_ptr<Memory::Containers::B_Chunks> data)
{
    if (!varName.empty())
    {
        m_vars.insert(std::pair<std::string, std::shared_ptr<Memory::Containers::B_Chunks>>(boost::to_upper_copy(varName), data));
        return true;
    }
    return false;
}

size_t HTTP::URLVars::size()
{
    Memory::Streams::StreamableNull nullobj;
    streamTo(&nullobj);
    if (nullobj.writeStatus.succeed == true)
    {
        return nullobj.writeStatus.bytesWritten;
    }
    return 0;
}

void HTTP::URLVars::iSetMaxVarContentSize()
{
    if (m_currentStat == URLV_STAT_WAITING_CONTENT)
        m_urlVarParser.setMaxObjectSize(m_maxVarContentSize);
}

void HTTP::URLVars::iSetMaxVarNameSize()
{
    if (m_currentStat == URLV_STAT_WAITING_NAME)
        m_urlVarParser.setMaxObjectSize(m_maxVarNameSize);
}
