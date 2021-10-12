#include "random.h"

#include <random>

using namespace std;
using namespace CX2::Helpers;

Random::Random()
{
}

string Random::createRandomString(string::basic_string::size_type length)
{
    // TODO: check randomness for crypto!
    char baseChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string randomStr;
    std::mt19937 rg{std::random_device{}()};
    std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(baseChars)-2);
    randomStr.reserve(length);
    while(length--) randomStr += baseChars[pick(rg)];
    return randomStr;
}

void Random::createRandomSalt32(unsigned char *salt)
{
    std::mt19937 rg{std::random_device{}()};
    std::uniform_int_distribution<uint32_t> pick;
    *((uint32_t *)salt) = pick(rg);
}
