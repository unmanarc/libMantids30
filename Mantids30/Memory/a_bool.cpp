#include "a_bool.h"
#include <shared_mutex>
#include <mutex>

using namespace Mantids30::Memory::Abstract;

BOOL::BOOL()
{
    setVarType(Type::BOOL);
}

BOOL::BOOL(const bool &value)
{
    setVarType(Type::BOOL);
    this->m_value = value;
}

bool BOOL::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_value;
}

bool BOOL::setValue(bool value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    this->m_value = value;
    return true;
}

std::string BOOL::toString()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_value ? "true" : "false";
}

bool BOOL::fromString(const std::string &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    this->m_value = value == "true" || value == "TRUE" || value == "1" || value == "t" || value == "T";
    return true;
}

Json::Value BOOL::toJSON()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    if (isNull())
    {
        return Json::nullValue;
    }

    return m_value;
}

bool BOOL::fromJSON(const Json::Value &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_value = Helpers::JSON::ASBOOL_D(value, false);
    return true;
}

std::shared_ptr<Var> BOOL::protectedCopy()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::shared_ptr<BOOL> var = std::make_shared<BOOL>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}
