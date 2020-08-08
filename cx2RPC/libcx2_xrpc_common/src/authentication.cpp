#include "authentication.h"

using namespace CX2::RPC;
using namespace CX2;

Authentication::Authentication()
{
    passIndex = 0;
}

Authentication::Authentication(const std::string &pass, const uint32_t &idx)
{
    userPass = pass;
    passIndex = idx;
}

bool Authentication::fromJSON(const Json::Value &x)
{
    if (!x["pass"].isNull()) userPass = x["pass"].asString();
    else return false;
    if (!x["idx"].isNull()) passIndex = x["idx"].asUInt();
    else return false;
    return true;
}

Json::Value Authentication::toJSON() const
{
    Json::Value x;
    x["pass"] = userPass;
    x["idx"] = passIndex;
    return x;
}

std::string Authentication::getUserPass() const
{
    return userPass;
}

void Authentication::setUserPass(const std::string &value)
{
    userPass = value;
}

uint32_t Authentication::getPassIndex() const
{
    return passIndex;
}

void Authentication::setPassIndex(const uint32_t &value)
{
    passIndex = value;
}
