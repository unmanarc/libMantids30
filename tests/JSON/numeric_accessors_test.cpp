#include <Mantids30/DataFormat/JSON/json.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

using namespace Mantids30::DataFormat::JSON;

static void require(bool condition, const char *message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

template<typename T>
static void checkIntegers(T (*direct)(const boost::json::value &, T),
                          T (*member)(const boost::json::value &, std::string_view, T),
                          T (*element)(const boost::json::value &, size_t, T))
{
    const T fallback = 37;
    const auto check = [&](const boost::json::value &value, T expected)
    {
        const boost::json::value object = boost::json::object{{"n", value}};
        const boost::json::value array = boost::json::array{value};
        require(direct(value, fallback) == expected, "Direct integer conversion mismatch");
        require(member(object, "n", fallback) == expected, "Member integer conversion mismatch");
        require(element(array, 0, fallback) == expected, "Array integer conversion mismatch");
    };
    check(nullptr, fallback);
    check(true, fallback);
    check("123", fallback);
    check(boost::json::object{}, fallback);
    check(boost::json::array{}, fallback);
    check(0.0, fallback);
    check(12.5, fallback);
    check(std::numeric_limits<double>::max(), fallback);
    check(std::numeric_limits<double>::infinity(), fallback);
    check(std::numeric_limits<double>::quiet_NaN(), fallback);
    check(int64_t(0), 0);
    check(uint64_t(0), 0);
    check(int64_t(42), 42);
    check(uint64_t(42), 42);
    check(std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest());
    check(std::numeric_limits<T>::max(), std::numeric_limits<T>::max());
    if constexpr (std::is_signed_v<T>)
    {
        check(int64_t(-1), -1);
        check(static_cast<uint64_t>(std::numeric_limits<T>::max()), std::numeric_limits<T>::max());
        check(static_cast<uint64_t>(std::numeric_limits<T>::max()) + 1, fallback);
        check(std::numeric_limits<uint64_t>::max(), fallback);
        if constexpr (sizeof(T) < sizeof(int64_t))
        {
            check(static_cast<int64_t>(std::numeric_limits<T>::lowest()) - 1, fallback);
            check(static_cast<int64_t>(std::numeric_limits<T>::max()) + 1, fallback);
        }
    }
    else
    {
        check(int64_t(-1), fallback);
        check(std::numeric_limits<int64_t>::lowest(), fallback);
        if constexpr (sizeof(T) < sizeof(uint64_t))
        {
            check(static_cast<uint64_t>(std::numeric_limits<T>::max()) + 1, fallback);
            check(static_cast<int64_t>(std::numeric_limits<T>::max()) + 1, fallback);
        }
    }
    require(member(boost::json::object{}, "absent", fallback) == fallback, "Missing member fallback mismatch");
    require(element(boost::json::array{}, 0, fallback) == fallback, "Missing element fallback mismatch");
    require(member(nullptr, "n", fallback) == fallback, "Non-object fallback mismatch");
    require(element(nullptr, 0, fallback) == fallback, "Non-array fallback mismatch");
}

int main()
{
    try
    {
        checkIntegers(ASINT_D, ASINT, ARRAY_ASINT);
        checkIntegers(ASUINT_D, ASUINT, ARRAY_ASUINT);
        checkIntegers(ASINT64_D, ASINT64, ARRAY_ASINT64);
        checkIntegers(ASUINT64_D, ASUINT64, ARRAY_ASUINT64);
        const float fallback = 37.0f;
        const auto checkFloat = [&](const boost::json::value &value, float expected)
        {
            require(ASFLOAT_D(value, fallback) == expected, "Direct float conversion mismatch");
            require(ASFLOAT(boost::json::object{{"n", value}}, "n", fallback) == expected, "Member float conversion mismatch");
            require(ARRAY_ASFLOAT(boost::json::array{value}, 0, fallback) == expected, "Array float conversion mismatch");
        };
        checkFloat(nullptr, fallback);
        checkFloat(1.5, 1.5f);
        checkFloat(int64_t(-42), -42.0f);
        checkFloat(uint64_t(42), 42.0f);
        checkFloat(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        checkFloat(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
        checkFloat(std::nextafter(static_cast<double>(std::numeric_limits<float>::max()),
                                 std::numeric_limits<double>::infinity()), fallback);
        checkFloat(std::numeric_limits<double>::max(), fallback);
        checkFloat(std::numeric_limits<double>::lowest(), fallback);
        checkFloat(std::numeric_limits<double>::infinity(), fallback);
        checkFloat(-std::numeric_limits<double>::infinity(), fallback);
        checkFloat(std::numeric_limits<double>::quiet_NaN(), fallback);
        require(ASINT64_OR_ZERO(std::numeric_limits<uint64_t>::max()) == 0, "Signed zero fallback mismatch");
        require(ASUINT64_OR_ZERO(-1) == 0, "Unsigned zero fallback mismatch");
        const boost::json::value values = boost::json::array{0, 42, std::numeric_limits<uint32_t>::max(),
            uint64_t(std::numeric_limits<uint32_t>::max()) + 1, -1, 1.5, "7", nullptr};
        const std::set<uint32_t> expected{0, 42, std::numeric_limits<uint32_t>::max()};
        require(toUInt32Set(values) == expected, "Invalid integers were not skipped in set conversion");
        require(toUInt32Set(boost::json::object{{"n", values}}, "n") == expected, "Member set conversion mismatch");
        std::cout << "JSON numeric accessor checks passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
