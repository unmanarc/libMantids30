#include "a_double.h"
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <locale>
#include <locale.h>
#include <memory>
#include <mutex>
#include <type_traits>

using namespace Mantids30::Core::Memory::Abstract;

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
    if (value.empty() || value.front() == '+' || std::isspace(value.front(), std::locale::classic()))
    {
        return false;
    }

    const auto magnitude = value.substr(value.front() == '-' ? 1 : 0);
    if (magnitude.substr(0, 2) == "0x" || magnitude.substr(0, 2) == "0X")
    {
        return false;
    }

    // GCC 8 lacks floating-point from_chars. Use a private C locale without
    // changing the process locale, and own a terminated copy of the view.
#ifdef _WIN32
    static const std::unique_ptr<std::remove_pointer_t<_locale_t>, decltype(&_free_locale)>
        numericLocale(_create_locale(LC_NUMERIC, "C"), &_free_locale);
#else
    static const std::unique_ptr<std::remove_pointer_t<locale_t>, decltype(&freelocale)>
        numericLocale(newlocale(LC_NUMERIC_MASK, "C", nullptr), &freelocale);
#endif
    if (!numericLocale)
    {
        return false;
    }

    const std::string text(value);
    char *end = nullptr;
    errno = 0;
#ifdef _WIN32
    const double parsed = _strtod_l(text.c_str(), &end, numericLocale.get());
#else
    const double parsed = strtod_l(text.c_str(), &end, numericLocale.get());
#endif
    // Representable subnormals may set ERANGE; underflow to zero must fail.
    if (end == text.c_str() || end != text.c_str() + text.size() ||
        (errno == ERANGE && (parsed == 0.0 || std::isinf(parsed))))
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
    m_value = Mantids30::DataFormat::JSON::ASDOUBLE_D(value, 0);
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
