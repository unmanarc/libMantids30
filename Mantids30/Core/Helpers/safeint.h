#pragma once

#include <limits>
#include <stdexcept>

// Safe Add
template<typename T>
T safe_add(T a, T b)
{
    if (a > 0 && b > 0 && std::numeric_limits<T>::max() - a < b)
    {
        throw std::overflow_error("Safe Add: Overflow detected");
    }
    if (a < 0 && b < 0 && std::numeric_limits<T>::min() - a > b)
    {
        throw std::underflow_error("Safe Add: Underflow detected");
    }
    return a + b;
}

// Safe Subtract
template<typename T>
T safe_sub(T a, T b)
{
    if (b > 0 && a < std::numeric_limits<T>::min() + b)
    {
        throw std::underflow_error("Safe Subtract: Underflow detected");
    }
    if (b < 0 && a > std::numeric_limits<T>::max() + b)
    {
        throw std::overflow_error("Safe Subtract: Overflow detected");
    }
    return a - b;
}

// Safe Multiply
template<typename T>
T safe_mul(T a, T b)
{
    if (a > 0 && b > 0 && a > std::numeric_limits<T>::max() / b)
    {
        throw std::overflow_error("Safe Multiply: Overflow detected");
    }
    if (a < 0 && b < 0 && a < std::numeric_limits<T>::max() / b)
    {
        throw std::overflow_error("Safe Multiply: Overflow detected");
    }
    if (a > 0 && b < 0 && b < std::numeric_limits<T>::min() / a)
    {
        throw std::underflow_error("Safe Multiply: Underflow detected");
    }
    if (a < 0 && b > 0 && a < std::numeric_limits<T>::min() / b)
    {
        throw std::underflow_error("Safe Multiply: Underflow detected");
    }
    return a * b;
}

// Safe Divide
template<typename T>
T safe_div(T a, T b)
{
    if (b == 0)
    {
        throw std::invalid_argument("Safe Divide: Division by zero");
    }
    if (a == std::numeric_limits<T>::min() && b == -1)
    {
        throw std::overflow_error("Safe Divide: Overflow detected");
    }
    return a / b;
}

// Forward declaration
template<class To, class From>
constexpr bool in_range_17(From value);

/**
 * Performs a safe cast from From to To.
 * If the value is outside the representable range of To, returns default_value.
 * Only supports integral-to-integral conversions.
 */
template<class To, class From>
constexpr To safe_cast_or(From value, To default_value)
{
    static_assert(std::is_integral_v<To>, "To must be an integral type");
    static_assert(std::is_integral_v<From>, "From must be an integral type");
    static_assert(!std::is_same_v<To, From>, "To and From must be different types");

    return in_range_17<To>(value) ? static_cast<To>(value) : default_value;
}

/**
 * Checks if a value of type From can be safely represented in type To.
 * Only valid for integral types.
 */
template<class To, class From>
constexpr bool in_range_17(From value)
{
    static_assert(std::is_integral_v<To>, "To must be an integral type");
    static_assert(std::is_integral_v<From>, "From must be an integral type");

    using ToLimits = std::numeric_limits<To>;
    using FromLimits = std::numeric_limits<From>;

    constexpr bool to_signed = ToLimits::is_signed;
    constexpr bool from_signed = FromLimits::is_signed;

    if constexpr (to_signed && from_signed)
    {
        if constexpr (ToLimits::digits > FromLimits::digits)
        {
            return true;
        }

        return value >= static_cast<From>(ToLimits::min()) && value <= static_cast<From>(ToLimits::max());
    }
    else if constexpr (to_signed && !from_signed)
    {
        if constexpr (ToLimits::digits > FromLimits::digits)
        {
            return true;
        }

        return value <= static_cast<From>(ToLimits::max());
    }
    else if constexpr (!to_signed && from_signed)
    {
        if (value < 0)
        {
            return false;
        }

        // Después de descartar negativos, si To tiene al menos tantos bits,
        // todos los valores positivos de From caben en To.
        if constexpr (ToLimits::digits >= FromLimits::digits)
        {
            return true;
        }

        return value <= static_cast<From>(ToLimits::max());
    }
    else
    {
        if constexpr (ToLimits::digits > FromLimits::digits)
        {
            return true;
        }

        return value <= static_cast<From>(ToLimits::max());
    }
}
/*
template<typename To, typename From>
To unsafeStaticCast(From value)
{
    if (value > static_cast<From>(std::numeric_limits<To>::max()))
    {
        return To{};
    }
    if (value < static_cast<From>(std::numeric_limits<To>::min()))
    {
        return To{};
    }
    return static_cast<To>(value);
}
*/
