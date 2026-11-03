#include "a_ptr.h"
#include <mutex>
#include <cstdint>

using namespace Mantids30::Memory::Abstract;

PTR::PTR()
{
    setVarType(Type::PTR);
}

PTR::PTR(void *value)
{
    setVarType(Type::PTR);
    this->m_value = value;
}

void *PTR::getValue()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_value;
}

bool PTR::setValue(void *value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    this->m_value = value;
    return true;
}

std::string PTR::toString()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    char ovalue[256];
    const void *ptr = m_value;
    snprintf(ovalue, sizeof(ovalue), "%.8lX", (uintptr_t) ptr);
    return ovalue;
}

bool PTR::fromString(const std::string &value)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    if (value.empty())
    {
        this->m_value = nullptr;
        return true;
    }
    this->m_value = (void *) (strtol(value.c_str(), nullptr, 16));
    return true;
}

std::shared_ptr<Var> PTR::protectedCopy()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::shared_ptr<PTR> var = std::make_shared<PTR>();
    if (var)
    {
        *var = this->m_value;
    }
    return var;
}

boost::json::value PTR::toJSON()
{
    if (isNull())
    {
        return boost::json::value{};
    }

    return boost::json::value(toString());
}

bool PTR::fromJSON(const boost::json::value &value)
{
    return fromString(Helpers::JSON::ASSTRING_D(value, "00000000"));
}
