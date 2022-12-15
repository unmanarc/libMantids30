#include "data.h"

using namespace Mantids::Authentication;
using namespace Mantids;

Data::Data()
{
    iPassIDX = 0;
}

Data::Data(const std::string &pass, const uint32_t &idx)
{
    sPassword = pass;
    iPassIDX = idx;
}

bool Data::fromString(const std::string &sAuth)
{
     json x;

     if (sAuth.empty()) return true;

     Mantids::Helpers::JSONReader2 reader;
     if (!reader.parse(sAuth, x)) return false;
     if (!x.isObject()) return false;

     return fromJSON(x);
}

bool Data::fromJSON(const json &x)
{
//    if (!x["user"].isNull()) userName = JSON_ASSTRING(x,"user","");

    if (x["pass"].isNull() || x["idx"].isNull())
        return false;

    sPassword = JSON_ASSTRING(x,"pass","");
    iPassIDX = JSON_ASUINT(x,"idx",0);

    return true;
}

json Data::toJSON() const
{
    json x;
    x["pass"] = sPassword;
    x["idx"] = iPassIDX;
    return x;
}

std::string Data::getPassword() const
{
    return sPassword;
}

void Data::setPassword(const std::string &value)
{
    sPassword = value;
}

uint32_t Data::getPassIndex() const
{
    return iPassIDX;
}

void Data::setPassIndex(const uint32_t &value)
{
    iPassIDX = value;
}
/*
std::string Data::getUserName() const
{
    return userName;
}

void Data::setUserName(const std::string &value)
{
    userName = value;
}
*/
