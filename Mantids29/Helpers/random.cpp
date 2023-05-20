#include "random.h"

#include <openssl/rand.h>
#include <stdexcept>

using namespace std;
using namespace Mantids29::Helpers;

Random::Random() {}

std::string Random::createRandomString(size_t length)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const size_t max_index = (sizeof(charset) - 1);

    std::string randomString(length, '\0');
    unsigned char buffer[length];

    if (RAND_bytes(buffer, length)) {
        for (size_t i = 0; i < length; ++i) {
            randomString[i] = charset[buffer[i] % max_index];
        }
    } else {
        throw std::runtime_error("RAND_bytes failed.");
    }

    return randomString;
}

std::string Random::createRandomHexString(size_t length)
{
    length = length * 2;
    const char charset[] = "ABCDEF0123456789";
    const size_t max_index = (sizeof(charset) - 1);

    std::string randomString(length, '\0');
    unsigned char buffer[length];

    if (RAND_bytes(buffer, length)) {
        for (size_t i = 0; i < length; ++i) {
            randomString[i] = charset[buffer[i] % max_index];
        }
    } else {
        throw std::runtime_error("RAND_bytes failed.");
    }

    return randomString;
}

void Random::createRandomSalt32(unsigned char *salt)
{
    if (!RAND_bytes(salt, sizeof(uint32_t))) {
        throw std::runtime_error("RAND_bytes failed.");
    }
}

void Random::createRandomSalt128(unsigned char *salt)
{
    if (!RAND_bytes(salt, 16)) {
        throw std::runtime_error("RAND_bytes failed.");
    }
}
