#include "multiauths.h"
//#include "retcodes.h"

using namespace Mantids::RPC;
using namespace Mantids;

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
        Memory::Streams::StreamableJSON s;
        s.setValue(i.second.toJSON());
        std::cout << ">>>> With auth: " << s.getString() << std::endl << std::flush;
    }
}

bool MultiAuths::setAuthentications(const std::string &sAuthentications)
{
    if (sAuthentications.empty()) return true;

    json jAuthentications;
    Mantids::Helpers::JSONReader2 reader;
    if (!reader.parse(sAuthentications, jAuthentications)) return false;
    if (!jAuthentications.isObject()) return false;

    if (jAuthentications.isObject())
    {
        for (const auto &idx : jAuthentications.getMemberNames())
        {
            if ( jAuthentications[idx].isMember("pass") )
            {
                addAuthentication(strtoul(idx.c_str(),nullptr,10), JSON_ASSTRING(jAuthentications[idx],"pass",""));
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
