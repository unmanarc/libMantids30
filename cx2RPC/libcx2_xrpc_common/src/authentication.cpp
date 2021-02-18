#include "authentication.h"

using namespace CX2::RPC;
using namespace CX2;

Authentication::Authentication()
{
    iPassIDX = 0;
}

Authentication::Authentication(const std::string &pass, const uint32_t &idx)
{
    sPassword = pass;
    iPassIDX = idx;
}

bool Authentication::fromString(const std::string &sAuth)
{
     Json::Value x;

     if (sAuth.empty()) return true;

     Json::Reader reader;
     if (!reader.parse(sAuth, x)) return false;
     if (!x.isObject()) return false;

     return fromJSON(x);
}

bool Authentication::fromJSON(const Json::Value &x)
{
//    if (!x["user"].isNull()) userName = x["user"].asString();

    if (!x["pass"].isNull()) sPassword = x["pass"].asString();
    else return false;

    if (!x["idx"].isNull()) iPassIDX = x["idx"].asUInt();
    else return false;

    return true;
}

Json::Value Authentication::toJSON() const
{
    Json::Value x;
    x["pass"] = sPassword;
    x["idx"] = iPassIDX;
    return x;
}

std::string Authentication::getPassword() const
{
    return sPassword;
}

void Authentication::setPassword(const std::string &value)
{
    sPassword = value;
}

uint32_t Authentication::getPassIndex() const
{
    return iPassIDX;
}

void Authentication::setPassIndex(const uint32_t &value)
{
    iPassIDX = value;
}
/*
std::string Authentication::getUserName() const
{
    return userName;
}

void Authentication::setUserName(const std::string &value)
{
    userName = value;
}
*/
