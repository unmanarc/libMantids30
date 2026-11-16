#pragma once

#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace MemoryTests {

struct Test
{
    std::string suite, name;
    std::function<void()> run;
};
// Share one registry across translation units so every named case reaches the runner.
inline std::vector<Test> &registry()
{
    static std::vector<Test> tests;
    return tests;
}
struct Register
{
    Register(const char *suite, const char *name, std::function<void()> run)
    {
        registry().push_back({suite, name, std::move(run)});
    }
};

// Throw instead of using assert so checks remain active in release builds.
inline void check(bool result, const char *expression, const char *file, int line)
{
    if (!result)
    {
        throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + ": " + expression);
    }
}

template <class Exception, class Function>
void throws(Function action)
{
    try
    {
        action();
    }
    catch (const Exception &)
    {
        return;
    }
    throw std::runtime_error("Expected exception was not thrown");
}

// Include null and high-bit bytes in a reproducible pattern for binary boundary checks.
inline std::string binaryData(size_t size)
{
    std::string data(size, '\0');
    for (size_t i = 0; i < size; ++i)
    {
        data[i] = static_cast<char>((i * 31 + i / 251) % 256);
    }
    return data;
}

// Each case owns its directory; parallel CTest runs never share filenames.
class TempDirectory
{
public:
    TempDirectory()
    {
        std::random_device random;
        for (int attempt = 0; attempt < 100; ++attempt)
        {
            path = std::filesystem::temp_directory_path() /
                   ("mantids30-memory-test-" + std::to_string(random()) + "-" + std::to_string(random()));
            if (std::filesystem::create_directory(path))
            {
                return;
            }
        }
        throw std::runtime_error("Cannot create test temporary directory");
    }
    ~TempDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
    TempDirectory(const TempDirectory &) = delete;
    TempDirectory &operator=(const TempDirectory &) = delete;
    std::filesystem::path path;
};

constexpr size_t unlimited = std::numeric_limits<size_t>::max();

} // namespace MemoryTests

#define CHECK(...) MemoryTests::check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __FILE__, __LINE__)
#define MEMORY_JOIN_IMPL(a, b) a##b
#define MEMORY_JOIN(a, b) MEMORY_JOIN_IMPL(a, b)
#define TEST(suite, name) \
    static void MEMORY_JOIN(test_, __LINE__)(); \
    static MemoryTests::Register MEMORY_JOIN(registration_, __LINE__)(suite, name, MEMORY_JOIN(test_, __LINE__)); \
    static void MEMORY_JOIN(test_, __LINE__)()
