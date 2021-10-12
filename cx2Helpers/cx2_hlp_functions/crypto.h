#ifndef CRYPTO_H
#define CRYPTO_H


#include <string>

namespace CX2 { namespace Helpers {


class Crypto
{
public:
    Crypto();
    static std::string calcSHA256(const std::string & password);
    static std::string calcSHA512(const std::string & password);

    // SALT must be 4 bytes.
    static std::string calcSSHA256(const std::string & password, const unsigned char *ssalt);
    static std::string calcSSHA512(const std::string & password, const unsigned char *ssalt);
};

}}

#endif // CRYPTO_H
