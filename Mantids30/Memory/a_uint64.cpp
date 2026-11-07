#include "a_uint64.h"
#include <mutex>

using namespace Mantids30::Memory::Abstract;

UINT64::UINT64()
{
    setVarType(Type::UINT64);
}

UINT64::UINT64(const uint64_t &value)
{
    setVarType(Type::UINT64);
    this->m_value = value;
}

uint64_t UINT64::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    return m_value;
}

int64_t UINT64::getIValueTruncatedOrZero()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    if (m_value <= 0x7FFFFFFFFFFFFFFF)
    {
        return m_value;
    }
    else
    {
        return 0;
    }
}

bool UINT64::setValue(const uint64_t &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string UINT64::toString()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    return std::to_string(m_value);
}

bool UINT64::fromString(const std::string_view value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    uint64_t result = 0;

    const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result, 10);

    if (ec != std::errc{} || ptr != value.data() + value.size())
    {
        m_value = 0;
        return false;
    }

    m_value = result;
    return true;
}

std::shared_ptr<Var> UINT64::protectedCopy()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::shared_ptr<UINT64> var = std::make_shared<UINT64>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}

boost::json::value UINT64::toJSON()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    if (isNull())
    {
        return boost::json::value{};
    }

    return m_value;
}

bool UINT64::fromJSON(const boost::json::value &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_value = Helpers::JSON::ASUINT64_D(value, 0);
    return true;
}
