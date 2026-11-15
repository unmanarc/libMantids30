#include "a_uint32.h"
#include <mutex>

using namespace Mantids30::Memory::Abstract;

UINT32::UINT32()
{
    setVarType(Type::UINT32);
}

UINT32::UINT32(const uint32_t &value)
{
    setVarType(Type::UINT32);
    this->m_value = value;
}

uint32_t UINT32::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    return m_value;
}

bool UINT32::setValue(const uint32_t &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string UINT32::toString()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    return std::to_string(m_value);
}

bool UINT32::fromString(const std::string_view value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    uint32_t result = 0;

    const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result, 10);

    if (ec != std::errc{} || ptr != value.data() + value.size())
    {
        m_value = 0;
        return false;
    }

    m_value = result;
    return true;
}

std::shared_ptr<Var> UINT32::protectedCopy()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::shared_ptr<UINT32> var = std::make_shared<UINT32>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}

boost::json::value UINT32::toJSON()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    if (isNull())
    {
        return boost::json::value{};
    }

    return m_value;
}

bool UINT32::fromJSON(const boost::json::value &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_value = Helpers::JSON::ASUINT_D(value, 0);
    return true;
}
