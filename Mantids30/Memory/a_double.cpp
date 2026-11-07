#include "a_double.h"
#include <mutex>

#include <stdexcept> // std::invalid_argument

using namespace Mantids30::Memory::Abstract;

DOUBLE::DOUBLE()
{
    setVarType(Type::DOUBLE);
}

DOUBLE::DOUBLE(const double &value)
{
    setVarType(Type::DOUBLE);

    this->m_value = value;
}

double DOUBLE::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_value;
}

void DOUBLE::setValue(const double &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    this->m_value = value;
}

std::string DOUBLE::toString()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return std::to_string(m_value);
}

bool DOUBLE::fromString(std::string_view value)
{
    double parsed;

    const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), parsed);

    if (ec != std::errc{} || ptr != value.data() + value.size())
    {
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_value = parsed;

    return true;
}

boost::json::value DOUBLE::toJSON()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if (isNull())
    {
        return boost::json::value{};
    }

    return m_value;
}

bool DOUBLE::fromJSON(const boost::json::value &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_value = Helpers::JSON::ASDOUBLE_D(value, 0);
    return true;
}

std::shared_ptr<Var> DOUBLE::protectedCopy()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::shared_ptr<DOUBLE> var = std::make_shared<DOUBLE>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}
