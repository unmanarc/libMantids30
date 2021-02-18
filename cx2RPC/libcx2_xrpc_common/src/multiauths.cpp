#include "multiauths.h"
//#include "retcodes.h"

using namespace CX2::RPC;
using namespace CX2;

MultiAuths::MultiAuths()
{
    clear();
}


std::set<uint32_t> MultiAuths::getAuthenticationsIdxs()
{
    std::set<uint32_t> r;
    for (const auto & i : authentications)
        r.insert(i.first);
    return r;
}

Authentication MultiAuths::getAuthentication(const uint32_t &idx)
{
    if (authentications.find(idx) != authentications.end())
        return authentications[idx];

    Authentication r;
    return r;
}


void MultiAuths::print()
{
    for (const auto & i : authentications)
    {
        Memory::Streams::JSON_Streamable s;
        s.setValue(i.second.toJSON());
        std::cout << ">>>> With auth: " << s.getString() << std::endl << std::flush;
    }
}

bool MultiAuths::setAuthentications(const std::string &sAuthentications)
{
    if (sAuthentications.empty()) return true;

    Json::Value jAuthentications;
    Json::Reader reader;
    if (!reader.parse(sAuthentications, jAuthentications)) return false;
    if (!jAuthentications.isObject()) return false;

    if (jAuthentications.isObject())
    {
        for (const auto &idx : jAuthentications.getMemberNames())
        {
            if ( jAuthentications[idx].isMember("pass") )
            {
                addAuthentication(strtoul(idx.c_str(),nullptr,10), jAuthentications[idx]["pass"].asString());
            }
        }
    }

    return true;
}


void MultiAuths::clear()
{
    authentications.clear();
}

void MultiAuths::addAuthentication(const Authentication &auth)
{
    authentications[auth.getPassIndex()] = auth;
}

void MultiAuths::addAuthentication(uint32_t passIndex,const std::string &pass)
{
    authentications[passIndex].setPassIndex(passIndex);
    authentications[passIndex].setPassword(pass);
}
