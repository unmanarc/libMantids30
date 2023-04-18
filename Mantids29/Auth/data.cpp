#include "data.h"

using namespace Mantids29::Authentication;
using namespace Mantids29;

Data::Data()
{
}

Data::Data(const std::string& password, const uint32_t& passwordIndex)
{
    this->m_password = password;
    this->m_passwordIndex = passwordIndex;
}

bool Data::setJsonString(const std::string &sAuth)
{
     json x;

     if (sAuth.empty()) return true;

     Mantids29::Helpers::JSONReader2 reader;

     if (!reader.parse(sAuth, x)) return false;
     if (!x.isObject()) return false;

     return setJson(x);
}

bool Data::setJson(const json &jsonObject)
{
    if (jsonObject["pass"].isNull() || jsonObject["idx"].isNull())
        return false;

    this->m_password = JSON_ASSTRING(jsonObject,"pass","");
    this->m_passwordIndex = JSON_ASUINT(jsonObject,"idx",0);

    return true;
}

json Data::toJson() const
{
    json x;
    x["pass"] = this->m_password;
    x["idx"] = this->m_passwordIndex;
    return x;
}
