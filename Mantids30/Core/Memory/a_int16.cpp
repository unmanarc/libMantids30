#include "a_int16.h"
#include <mutex>
#include <cstdlib>

using namespace Mantids30::Memory::Abstract;

INT16::INT16()
{
    setVarType(Type::INT16);
}

INT16::INT16(const int16_t &value)
{
    this->m_value = value;

    setVarType(Type::INT16);
}

int16_t INT16::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    return m_value;
}

bool INT16::setValue(const int16_t &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string INT16::toString()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return std::to_string(m_value);
}

bool INT16::fromString(const std::string_view value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    int16_t result = 0;

    const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result, 10);

    if (ec != std::errc{} || ptr != value.data() + value.size())
    {
        m_value = 0;
        return false;
    }

    m_value = result;
    return true;
}

std::shared_ptr<Var> INT16::protectedCopy()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::shared_ptr<INT16> var = std::make_shared<INT16>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}

boost::json::value INT16::toJSON()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    if (isNull())
    {
        return boost::json::value{};
    }

    return m_value;
}

bool INT16::fromJSON(const boost::json::value &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_value = Helpers::JSON::ASINT_D(value, 0);
    return true;
}
