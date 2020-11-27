#include "crypto.h"
#include "encoders.h"
#include <openssl/sha.h>


using namespace CX2::Helpers;

Crypto::Crypto()
{
}

std::string Crypto::calcSHA256(const std::string &password)
{
    std::string r;
    unsigned char buffer_sha2[SHA256_DIGEST_LENGTH+1];
    SHA256_CTX sha2;
    SHA256_Init (&sha2);
    SHA256_Update (&sha2, (const unsigned char *) password.c_str(), password.length());
    SHA256_Final ( buffer_sha2, &sha2);
    return Encoders::toHex(buffer_sha2,SHA256_DIGEST_LENGTH);
}

std::string Crypto::calcSHA512(const std::string &password)
{
    std::string r;
    unsigned char buffer_sha2[SHA512_DIGEST_LENGTH+1];
    SHA512_CTX sha2;
    SHA512_Init (&sha2);
    SHA512_Update (&sha2, (const unsigned char *) password.c_str(), password.length());
    SHA512_Final ( buffer_sha2, &sha2);
    return Encoders::toHex(buffer_sha2,SHA512_DIGEST_LENGTH);
}

std::string Crypto::calcSSHA256(const std::string &password, const unsigned char * ssalt)
{
    std::string r;
    unsigned char buffer_sha2[SHA256_DIGEST_LENGTH+1];
    SHA256_CTX sha2;
    SHA256_Init (&sha2);
    SHA256_Update (&sha2, (const unsigned char *) password.c_str(), password.length());
    SHA256_Update (&sha2, ssalt, 4);
    SHA256_Final ( buffer_sha2, &sha2);
    return Encoders::toHex(buffer_sha2,SHA256_DIGEST_LENGTH);
}

std::string Crypto::calcSSHA512(const std::string &password, const unsigned char * ssalt)
{
    std::string r;
    unsigned char buffer_sha2[SHA512_DIGEST_LENGTH+1];
    SHA512_CTX sha2;
    SHA512_Init (&sha2);
    SHA512_Update (&sha2, (const unsigned char *) password.c_str(), password.length());
    SHA512_Update (&sha2, ssalt, 4);
    SHA512_Final ( buffer_sha2, &sha2);
    return Encoders::toHex(buffer_sha2,SHA512_DIGEST_LENGTH);
}
