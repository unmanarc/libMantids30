#ifndef SAFEINT_H
#define SAFEINT_H

#include <limits>
#include <stdexcept>
//int safeStaticCast(unsigned long value);

// Safe Add
template<typename T>
T safeAdd(
    T a, T b)
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
T safeSubtract(
    T a, T b)
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
T safeMultiply(
    T a, T b)
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
T safeDivide(
    T a, T b)
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

// Safe Static Cast
template<typename To, typename From>
To safeStaticCast(
    From value)
{
    if (value > static_cast<From>(std::numeric_limits<To>::max()))
    {
        throw std::out_of_range("Safe Static Cast: Value out of range for target type");
    }
    if (value < static_cast<From>(std::numeric_limits<To>::min()))
    {
        throw std::underflow_error("Safe Static Cast: Value out of range for target type");
    }
    return static_cast<To>(value);
}

template<typename To, typename From>
To unsafeStaticCast(From value) {
    if (value > static_cast<From>(std::numeric_limits<To>::max())) {
        return To{};
    }
    if (value < static_cast<From>(std::numeric_limits<To>::min())) {
        return To{};
    }
    return static_cast<To>(value);
}

#endif // SAFEINT_H
