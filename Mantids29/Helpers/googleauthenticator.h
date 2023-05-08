#pragma once

#include <string>

namespace Mantids29 { namespace Helpers { namespace TOTP {


/**
 * @class GoogleAuthenticator
 * @brief This class implements the Google Authenticator's Time-based One-Time Password (TOTP) algorithm.
 *
 * The GoogleAuthenticator class provides functions to generate a TOTP and to verify a given TOTP against the expected value.
 */
class GoogleAuthenticator {
public:

    /// Default constructor
    GoogleAuthenticator() = default;

    /// Default destructor
    ~GoogleAuthenticator() = default;

    /**
     * @brief Generates a TOTP for a given secret.
     * @param base32Secret The shared secret in base32 format.
     * @param interval The time step in seconds. Default is 30.
     * @param digits The number of digits in the generated TOTP. Default is 6.
     * @param position The time step position relative to the current time. 0 for the current time step, -1 for the previous, 1 for the next, etc.
     * @return The generated TOTP.
     */
    static std::string generateTOTP(const std::string& base32Secret, int position=0, int interval = 30, int digits = 6);

    /**
     * @brief Verifies a TOTP against the expected value for a given secret.
     * @param base32Secret The shared secret in base32 format.
     * @param tokenInput The TOTP to verify.
     * @param interval The time step in seconds. Default is 30.
     * @param digits The number of digits in the TOTP. Default is 6.
     * @param aperture The number of time steps before and after the current time step to consider for verification. Default is 1 (one step after and one before)
     * @return True if the TOTP is valid, false otherwise.
     */
    static bool verifyToken(const std::string& base32Secret, const std::string& tokenInput, int aperture = 1, int interval = 30, int digits = 6);

};

}}}
