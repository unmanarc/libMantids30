#include "a_int32.h"
#include <mutex>

using namespace Mantids30::Memory::Abstract;

INT32::INT32()
{
    setVarType(Type::INT32);
}

INT32::INT32(const int32_t &value)
{
    this->m_value = value;

    setVarType(Type::INT32);
}

int32_t INT32::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_value;
}

bool INT32::setValue(const int32_t &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string INT32::toString()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    return std::to_string(m_value);
}

bool INT32::fromString(const std::string_view value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    int32_t result = 0;

    const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result, 10);

    if (ec != std::errc{} || ptr != value.data() + value.size())
    {
        m_value = 0;
        return false;
    }

    m_value = result;
    return true;
}

std::shared_ptr<Var> INT32::protectedCopy()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::shared_ptr<INT32> var = std::make_shared<INT32>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}

boost::json::value INT32::toJSON()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    if (isNull())
    {
        return boost::json::value{};
    }

    return m_value;
}

bool INT32::fromJSON(const boost::json::value &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_value = Helpers::JSON::ASINT_D(value, 0);
    return true;
}
