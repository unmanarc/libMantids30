#include "test_support.h"

int main(int argc, char **argv)
{
    size_t total = 0, failed = 0;
    // An optional suite name lets CTest run each group independently.
    for (const auto &test : MemoryTests::registry())
    {
        if (argc > 1 && test.suite != argv[1])
        {
            continue;
        }
        ++total;
        try
        {
            test.run();
            std::cout << "[PASS] " << test.suite << "." << test.name << std::endl;
        }
        catch (const std::exception &error)
        {
            ++failed;
            std::cerr << "[FAIL] " << test.suite << "." << test.name << ": " << error.what() << std::endl;
        }
    }
    std::cout << total << " cases, " << failed << " failures\n";
    return total == 0 || failed != 0 ? 1 : 0;
}
