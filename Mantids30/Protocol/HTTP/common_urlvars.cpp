#include "common_urlvars.h"

#include "streamencoder_url.h"
#include <Mantids30/Core/Memory/streamable_null.h>
#include <boost/algorithm/string.hpp>
#include <memory>

using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Protocol;

using namespace Mantids30;

Mantids30::Protocol::HTTP::URLVars::URLVars(const std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> &value)
    : Mantids30::Core::Memory::Streams::Parser(value, false)
{
    m_urlVarParser.setVarType(true);
    m_urlVarParser.setMaxObjectSize(m_maxVarNameSize);
    m_currentSubParser = &m_urlVarParser;
}

std::shared_ptr<Mantids30::Protocol::HTTP::URLVars> Mantids30::Protocol::HTTP::URLVars::create(const std::shared_ptr<StreamableObject> &value)
{
    std::shared_ptr<URLVars> x = std::shared_ptr<URLVars>(new URLVars(value));
    x->initSubParser(&x->m_urlVarParser);
    x->m_initialized = x->initProtocol();
    return x;
}

bool Mantids30::Protocol::HTTP::URLVars::isEmpty()
{
    return m_vars.empty();
}

bool Mantids30::Protocol::HTTP::URLVars::streamTo(Mantids30::Core::Memory::Streams::StreamableObject *out)
{
    bool firstVar = true;

    for (const auto &i : m_vars)
    {
        if (firstVar)
        {
            firstVar = false;
        }
        else
        {
            out->writeString("&");
            if (!out->writeStatus.succeed)
            {
                return false;
            }
        }

        Mantids30::Core::Memory::Containers::B_Chunks varName;
        varName.append(i.first.c_str(), i.first.size());

        Mantids30::Protocol::HTTP::Encoders::URL varNameEncoder;
        varNameEncoder.transform(&varName, out);
        if (!out->writeStatus.succeed)
        {
            return false;
        }

        if ((i.second)->size())
        {
            out->writeString("=");

            if (!out->writeStatus.succeed)
            {
                return false;
            }

            Mantids30::Protocol::HTTP::Encoders::URL varNameEncoder2;
            varNameEncoder2.transform(i.second.get(), out);
            if (!out->writeStatus.succeed)
            {
                return false;
            }
        }
    }

    return true;
}

uint32_t Mantids30::Protocol::HTTP::URLVars::varCount(const std::string &varName)
{
    uint32_t x = 0;

    for (const auto &i : m_vars)
    {
        if (i.first == varName)
        {
            x++;
        }
    }

    return x;
}

std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> Mantids30::Protocol::HTTP::URLVars::getValue(const std::string &varName)
{
    for (const auto &i : m_vars)
    {
        if (i.first == varName)
        {
            return i.second;
        }
    }
    return nullptr;
}

std::list<std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject>> Mantids30::Protocol::HTTP::URLVars::getValues(const std::string &varName)
{
    std::list<std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject>> r;
    for (const auto &i : m_vars)
    {
        if (i.first == varName)
        {
            r.push_back(i.second);
        }
    }
    return r;
}

std::set<std::string> Mantids30::Protocol::HTTP::URLVars::getKeysList()
{
    std::set<std::string> r;
    for (const auto &i : m_vars)
    {
        r.insert(i.first);
    }
    return r;
}

bool Mantids30::Protocol::HTTP::URLVars::initProtocol()
{
    return true;
}

void Mantids30::Protocol::HTTP::URLVars::endProtocol() {}

bool Mantids30::Protocol::HTTP::URLVars::changeToNextParser()
{
    switch (m_currentStat)
    {
    case URLVarsParsingStatus::WAITING_FOR_NAME:
    {
        m_currentVarName = m_urlVarParser.getContentAsStringAndFlush();
        if (m_urlVarParser.getFoundDelimiter() == "&" || (m_urlVarParser.isStreamEnded() && m_urlVarParser.getFoundDelimiter() != "="))
        {
            // AMP / END:
            return addVar(m_currentVarName, m_urlVarParser.getContentAndFlush());
        }
        else
        {
            // EQUAL:
            m_currentStat = URLVarsParsingStatus::WAITING_FOR_CONTENT;
            m_urlVarParser.setVarType(false);
            m_urlVarParser.setMaxObjectSize(m_maxVarContentSize);
            return true;
        }
    }
    break;
    case URLVarsParsingStatus::WAITING_FOR_CONTENT:
    {
        bool r = addVar(m_currentVarName, m_urlVarParser.getContentAndFlush());
        m_currentStat = URLVarsParsingStatus::WAITING_FOR_NAME;
        m_urlVarParser.setVarType(true);
        m_urlVarParser.setMaxObjectSize(m_maxVarNameSize);
        return r;
    }
    break;
    default:
        return true;
    }
}

bool Mantids30::Protocol::HTTP::URLVars::addVar(const std::string &varName, const std::shared_ptr<Mantids30::Core::Memory::Containers::B_Chunks> &data)
{
    if (!varName.empty() && m_vars.size() < m_maxVarsCount)
    {
        m_vars.emplace_back(varName, data);
        return true;
    }
    return false;
}

size_t Mantids30::Protocol::HTTP::URLVars::size()
{
    Mantids30::Core::Memory::Streams::StreamableNull nullobj;
    streamTo(&nullobj);
    if (nullobj.writeStatus.succeed)
    {
        return nullobj.writeStatus.bytesWritten;
    }
    return 0;
}

void Mantids30::Protocol::HTTP::URLVars::iSetMaxVarContentSize()
{
    if (m_currentStat == URLVarsParsingStatus::WAITING_FOR_CONTENT)
    {
        m_urlVarParser.setMaxObjectSize(m_maxVarContentSize);
    }
}

void Mantids30::Protocol::HTTP::URLVars::iSetMaxVarNameSize()
{
    if (m_currentStat == URLVarsParsingStatus::WAITING_FOR_NAME)
    {
        m_urlVarParser.setMaxObjectSize(m_maxVarNameSize);
    }
}
