#include "totp.h"

#include "encoders.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <cstring>
#include <ctime>
//#include <cmath>
//#include <vector>

using namespace Mantids30::Helpers::OTP;

std::string TOTP::generateTOTP(
    const std::string &base32Secret, int position, unsigned int interval, unsigned int digits)
{
    // Decode the base32 secret
    std::string secret = Mantids30::Helpers::Encoders::decodeFromBase32(base32Secret);

    char hashSHA1[20]; // Array to hold SHA1 hash result
    long dynamicBinaryCode; // Dynamic Binary Code (DBC)

    // Calculate the current Unix timestamp, shifted by the position and interval
    time_t now = time(nullptr) + (position * interval);
    uint64_t timeSteps;

    // Calculate the number of time steps since Unix epoch
    if (interval == 0)
    {
        interval = 30; // Default time step size is 30 seconds
    }

    timeSteps = now / interval;

    // Prepare the counter
    std::string counter(sizeof(timeSteps), '\0');

    for (size_t i = 0; i < sizeof(timeSteps); i++)
        counter[i] = (timeSteps >> ((sizeof(timeSteps) - i - 1) * 8)) & 0xFF;

    // Calculate HMAC SHA1
    HMAC(EVP_sha1(), secret.c_str(), static_cast<int>(secret.length()), reinterpret_cast<const unsigned char *>(counter.c_str()), sizeof(timeSteps), reinterpret_cast<unsigned char *>(hashSHA1), NULL);

    // Calculate the dynamic binary code (DBC)
    uint8_t offset = hashSHA1[19] & 0x0f;
    dynamicBinaryCode = (((hashSHA1[offset] & 0x7f) << 24) | ((hashSHA1[offset + 1] & 0xff) << 16) | ((hashSHA1[offset + 2] & 0xff) << 8) | ((hashSHA1[offset + 3] & 0xff)));

    // Convert dynamic binary code (DBC) to the OTP
    std::string otp = std::to_string(dynamicBinaryCode);

    // If the OTP size is greater than required digits, reduce it to required digits
    if (otp.size() > digits)
        otp = otp.substr(otp.length() - digits);

    // Add leading zeros if necessary
    while (otp.size() < digits)
    {
        otp = "0" + otp;
    }

    return otp;
}

bool TOTP::verifyToken(
    const std::string &base32Secret, const std::string &tokenInput, int aperture, unsigned int interval, unsigned int digits)
{
    // Check the input token against all tokens from -aperture to +aperture
    for(int i = -aperture; i <= aperture; i++)
    {
        std::string otp = generateTOTP(base32Secret, i, interval, digits);
        if(tokenInput == otp)
        {
            return true;
        }
    }

    // If none of the tokens matched, return false
    return false;
}

