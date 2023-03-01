#include "multi.h"
//#include "retcodes.h"

using namespace Mantids29::Authentication;
using namespace Mantids29;

Multi::Multi()
{
    clear();
}


std::set<uint32_t> Multi::getAvailableIndices()
{
    std::set<uint32_t> r;
    for (const auto & i : m_authentications)
        r.insert(i.first);
    return r;
}

Data Multi::getAuthentication(const uint32_t &passwordIndex)
{
    if (m_authentications.find(passwordIndex) != m_authentications.end())
        return m_authentications[passwordIndex];

    Data r;
    return r;
}
/*

void Multi::print()
{
    for (const auto & i : authentications)
    {
        Memory::Streams::StreamableJSON s;
        s.setValue(i.second.toJson());
        std::cout << ">>>> With auth: " << s.getString() << std::endl << std::flush;
    }
}*/

bool Multi::setAuthentications(const std::string &sAuthentications)
{
    if (sAuthentications.empty()) return true;

    json jAuthentications;
    Mantids29::Helpers::JSONReader2 reader;
    if (!reader.parse(sAuthentications, jAuthentications)) return false;

    return setJson(jAuthentications);
}

bool Multi::setJson(const json &jAuthentications)
{
    if (!jAuthentications.isObject()) return false;

    if (jAuthentications.isObject())
    {
        for (const auto &passwordIndex : jAuthentications.getMemberNames())
        {
            if ( jAuthentications[passwordIndex].isMember("pass") )
            {
                addAuthentication(strtoul(passwordIndex.c_str(),nullptr,10), JSON_ASSTRING(jAuthentications[passwordIndex],"pass",""));
            }
        }
    }

    return true;
}

void Multi::clear()
{
    m_authentications.clear();
}

void Multi::addAuthentication(const Data &auth)
{
    m_authentications[auth.m_passwordIndex] = auth;
}

void Multi::addAuthentication(uint32_t passwordIndex, const std::string &password)
{
    m_authentications[passwordIndex].m_passwordIndex = passwordIndex;
    m_authentications[passwordIndex].m_password = password;
}
