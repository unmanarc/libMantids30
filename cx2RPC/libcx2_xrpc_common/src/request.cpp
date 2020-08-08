#include "request.h"
//#include "retcodes.h"

using namespace CX2::RPC;
using namespace CX2;

Request::Request()
{
    clear();
}
/*
std::string Request::getDomainName() const
{
    return domainName;
}

void Request::setDomainName(const std::string &value)
{
    domainName = value;
}

std::string Request::getUserName() const
{
    return userName;
}

void Request::setUserName(const std::string &value)
{
    userName = value;
}*/

std::set<uint32_t> Request::getAuthenticationsIdxs()
{
    std::set<uint32_t> r;
    for (const auto & i : authentications)
        r.insert(i.first);
    return r;
}

Authentication Request::getAuthentication(const uint32_t &idx)
{
    if (authentications.find(idx) != authentications.end())
        return authentications[idx];

    Authentication r;
    return r;
}

/*
Json::Value Request::toJSON()
{
    Json::Value r;

    r["payload"] = *payload.getValue();

    for (const auto & i : authentications)
    {
        r["auth"][std::to_string(i.first)] = i.second.toJSON();
    }

    return r;
}
*/
void Request::print()
{
    //std::cout << "REQ> With payload    : " << payload.getString() <<std::flush;

    for (const auto & i : authentications)
    {
        Memory::Streams::JSON_Streamable s;
        s.setValue(i.second.toJSON());
        std::cout << ">>>> With auth: " << s.getString() << std::endl << std::flush;
    }
}
/*
bool Request::setPayload(const std::string &payload)
{
    if (payload.empty()) return true;
    Json::Reader reader;
    return reader.parse(payload, (*this->payload.getValue()));
}*/

bool Request::setAuthentications(const std::string &sAuthentications)
{
    if (sAuthentications.empty()) return true;

    Json::Value jAuthentications;
    Json::Reader reader;
    if (!reader.parse(sAuthentications, jAuthentications)) return false;
    if (!jAuthentications.isObject()) return false;

    if (jAuthentications.isObject())
    {
        for (auto idx : jAuthentications.getMemberNames())
        {
            if ( jAuthentications[idx].isMember("pass") )
            {
                addAuthentication(strtoul(idx.c_str(),nullptr,10), jAuthentications[idx]["pass"].asString());
            }
        }
    }

    return true;
}
/*
void Request::setExtraInfo(const Json::Value &extraInfo)
{
    (*this->extraInfo.getValue()) = extraInfo;
}
*//*
void Request::setPayload(const Json::Value &payload)
{
    (*this->payload.getValue()) = payload;
}

Json::Value Request::getPayload()
{
    return (*this->payload.getValue());
}*/
/*
Json::Value Request::getExtraInfo()
{
    return (*this->extraInfo.getValue());
}

void Request::setMethodName(const std::string &value)
{
    methodName = value;
}

std::string Request::getMethodName() const
{
    return methodName;
}

std::string Request::getRpcMode() const
{
    return rpcMode;
}

void Request::setRpcMode(const std::string &value)
{
    rpcMode = value;
}*/

void Request::clear()
{
    //retcode = 0;
//    rpcMode = "EXEC";
 //   methodName.clear();
  //  payload.clear();
 //   ids.clear();
   // extraInfo.clear();
    authentications.clear();
  //  setReqId(0);
}

void Request::addAuthentication(const Authentication &auth)
{
    authentications[auth.getPassIndex()] = auth;
}

void Request::addAuthentication(uint32_t passIndex,const std::string &pass)
{
    authentications[passIndex].setPassIndex(passIndex);
    authentications[passIndex].setUserPass(pass);
}
