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
    m_urlVarParser.setVarType(true);

    setMaxVarNameSize(4096);
    setMaxVarContentSize(4096);

    m_currentParser = &m_urlVarParser;
}

std::shared_ptr<URLVars> URLVars::create(
    std::shared_ptr<StreamableObject> value)
{
    auto x = std::shared_ptr<URLVars>(new URLVars(value));
    x->initSubParser(&x->m_urlVarParser);
    x->m_initialized = x->initProtocol();
    return x;
}

bool URLVars::isEmpty()
{
    return m_vars.empty();
}

bool URLVars::streamTo(Memory::Streams::StreamableObject * out, Memory::Streams::WriteStatus &wrsStat)
{
    Memory::Streams::WriteStatus cur;
    bool firstVar = true;
    for (auto & i : m_vars)
    {
        if (firstVar)
            firstVar=false;
        else
        {
            if (!(cur+=out->writeString("&", wrsStat)).succeed)
                return false;
        }

        Memory::Containers::B_Chunks varName;
        varName.append(i.first.c_str(), i.first.size());

        Memory::Streams::Encoders::URL varNameEncoder;

        auto cur = varNameEncoder.transform(&varName,out);
        wrsStat+=cur;
        if (!cur.succeed || !cur.finish)
        {
            out->writeEOF(false);
            return false;
        }

        if ((i.second)->size())
        {
            if (!(cur+=out->writeString("=",wrsStat)).succeed)
                return false;

            Memory::Streams::Encoders::URL varNameEncoder2;
            auto cur = varNameEncoder2.transform(i.second.get(),out);
            wrsStat+=cur;
            if (!cur.finish || !cur.succeed)
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
    auto range = m_vars.equal_range(boost::to_upper_copy(varName));
    for (auto iterator = range.first; iterator != range.second;) i++;
    return i;
}

std::shared_ptr<Memory::Streams::StreamableObject> URLVars::getValue(const std::string &varName)
{
    auto range = m_vars.equal_range(boost::to_upper_copy(varName));

    for (auto iterator = range.first; iterator != range.second;)
        return iterator->second;

    return nullptr;
}

std::list<std::shared_ptr<Memory::Streams::StreamableObject> > URLVars::getValues(const std::string &varName)
{
    std::list<std::shared_ptr<Memory::Streams::StreamableObject> > r;
    auto range = m_vars.equal_range(boost::to_upper_copy(varName));

    for (auto iterator = range.first; iterator != range.second;)
        r.push_back(iterator->second);
    return r;
}

std::set<std::string> URLVars::getKeysList()
{
    std::set<std::string> r;
    for ( const auto & i : m_vars ) r.insert(i.first);
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

bool URLVars::addVar(
    const std::string &varName, std::shared_ptr<Memory::Containers::B_Chunks> data)
{
    if (!varName.empty())
    {
        m_vars.insert(std::pair<std::string, std::shared_ptr<Memory::Containers::B_Chunks>>(boost::to_upper_copy(varName), data));
        return true;
    }
    return false;
}


void URLVars::iSetMaxVarContentSize()
{
    if (m_currentStat == URLV_STAT_WAITING_CONTENT)
        m_urlVarParser.setMaxObjectSize(m_maxVarContentSize);
}

void URLVars::iSetMaxVarNameSize()
{
    if (m_currentStat == URLV_STAT_WAITING_NAME)
        m_urlVarParser.setMaxObjectSize(m_maxVarNameSize);
}
