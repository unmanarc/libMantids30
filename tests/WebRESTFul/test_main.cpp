#include "test_support.h"
#include <iostream>

int main(int argc, char **argv)
{
    using namespace WebRESTFulTests;
    if (argc < 2 || argc > 3)
    {
        std::cerr << "Usage: webrestful_test SUITE [CASE_SUBSTRING]\n";
        return 2;
    }
    const std::string suite = argv[1];
    auto cases = endpointCases(suite);
    const auto policies = policyCases(suite);
    const auto states = stateCases(suite);
    cases.insert(cases.end(), policies.begin(), policies.end());
    cases.insert(cases.end(), states.begin(), states.end());
    if (suite == "integration")
    {
        cases = integrationCases();
    }
    size_t executed = 0, failed = 0;
    for (const auto &test : cases)
    {
        if (argc == 3 && test.name.find(argv[2]) == std::string::npos)
        {
            continue;
        }
        ++executed;
        try
        {
            test.run();
        }
        catch (const std::exception &error)
        {
            ++failed;
            std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
        }
    }
    std::cout << suite << ": " << executed - failed << '/' << executed << " passed\n";
    return executed == 0 ? 2 : failed ? 1 : 0;
}
