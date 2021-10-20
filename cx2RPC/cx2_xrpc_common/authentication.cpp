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
     json x;

     if (sAuth.empty()) return true;

     CX2::Helpers::JSONReader2 reader;
     if (!reader.parse(sAuth, x)) return false;
     if (!x.isObject()) return false;

     return fromJSON(x);
}

bool Authentication::fromJSON(const json &x)
{
//    if (!x["user"].isNull()) userName = JSON_ASSTRING(x,"user","");

    if (x["pass"].isNull() || x["idx"].isNull())
        return false;

    sPassword = JSON_ASSTRING(x,"pass","");
    iPassIDX = JSON_ASUINT(x,"idx",0);

    return true;
}

json Authentication::toJSON() const
{
    json x;
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
