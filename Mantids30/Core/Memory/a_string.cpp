#include "a_string.h"
#include <mutex>
#include <string>

using namespace Mantids30::Memory::Abstract;

STRING::STRING()
{
    setVarType(Type::STRING);
}

STRING::STRING(const std::string &value)
{
    setVarType(Type::STRING);
    this->m_value = value;
}

STRING::STRING(std::string_view value)
{
    setVarType(Type::STRING);
    this->m_value = value;
}

STRING::STRING(const char *value)
{
    setVarType(Type::STRING);
    this->m_value = value;
}

std::string STRING::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_value;
}

bool STRING::setValue(const std::string &value)
{
    return fromString(value);
}

bool STRING::setValue(const char *value)
{
    if (!value)
    {
        return fromString("");
    }

    return fromString(value);
}

std::string STRING::toString()
{
    return getValue();
}

bool STRING::fromString(const std::string_view value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    this->m_value = value;
    return true;
}

std::shared_ptr<Var> STRING::protectedCopy()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::shared_ptr<STRING> var = std::make_shared<STRING>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}

boost::json::value STRING::toJSON()
{
    if (isNull())
    {
        return boost::json::value{};
    }

    return boost::json::string{toString()};
}

bool STRING::fromJSON(const boost::json::value &value)
{
    return fromString(Helpers::JSON::ASSTRING_D(value, ""));
}
