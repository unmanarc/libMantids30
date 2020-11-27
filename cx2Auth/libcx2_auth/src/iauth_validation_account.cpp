#include "iauth_validation_account.h"

#include <cx2_hlp_functions/encoders.h>
#include <cx2_hlp_functions/random.h>
#include <cx2_hlp_functions/crypto.h>

using namespace CX2::Authorization;
using namespace CX2::Authorization::DataStructs;
using namespace CX2::Authorization::Validation;

IAuth_Validation_Account::IAuth_Validation_Account()
{
}

IAuth_Validation_Account::~IAuth_Validation_Account()
{

}

sPasswordData IAuth_Validation_Account::genPassword(const std::string &passwordInput, const PasswordModes &passMode, bool forceExpiration, const time_t &expirationDate, uint32_t _2faSteps)
{
    sPasswordData r;

    r.passwordMode = passMode;
    r.forceExpiration = forceExpiration;
    r.expiration = expirationDate;

    switch (passMode)
    {
    case PASS_MODE_NOTFOUND:
    {
        // Do nothing...
    } break;
    case PASS_MODE_PLAIN:
    {
        r.hash = passwordInput;
    } break;
    case PASS_MODE_SHA256:
    {
        r.hash = Helpers::Crypto::calcSHA256(passwordInput);
    } break;
    case PASS_MODE_SHA512:
    {
        r.hash = Helpers::Crypto::calcSHA512(passwordInput);
    } break;
    case PASS_MODE_SSHA256:
    {
        CX2::Helpers::Random::createRandomSalt32(r.ssalt);
        r.hash = Helpers::Crypto::calcSSHA256(passwordInput, r.ssalt);
    } break;
    case PASS_MODE_SSHA512:
    {
        CX2::Helpers::Random::createRandomSalt32(r.ssalt);
        r.hash = Helpers::Crypto::calcSSHA512(passwordInput, r.ssalt);
    } break;
    case PASS_MODE_GAUTHTIME:
        r.hash = passwordInput;
        r.gAuthSteps = _2faSteps;
    }

    return r;
}

AuthReason IAuth_Validation_Account::validatePassword(const sPasswordData &storedPassword, const std::string &passwordInput, const std::string &cramSalt, AuthMode authMode)
{
    bool saltedHash = false;
    std::string toCompare;

    if (storedPassword.isPasswordExpired())
        return AUTH_REASON_EXPIRED_PASSWORD;

    switch (storedPassword.passwordMode)
    {
    case PASS_MODE_NOTFOUND:
        return AUTH_REASON_INTERNAL_ERROR;
    case PASS_MODE_PLAIN:
    {
        toCompare = passwordInput;
    } break;
    case PASS_MODE_SHA256:
    {
        toCompare = Helpers::Crypto::calcSHA256(passwordInput);
    } break;
    case PASS_MODE_SHA512:
    {
        toCompare = Helpers::Crypto::calcSHA512(passwordInput);
    } break;
    case PASS_MODE_SSHA256:
    {
        toCompare = Helpers::Crypto::calcSSHA256(passwordInput, storedPassword.ssalt);
        saltedHash = true;
    } break;
    case PASS_MODE_SSHA512:
    {
        toCompare = Helpers::Crypto::calcSSHA512(passwordInput, storedPassword.ssalt);
        saltedHash = true;
    } break;
    case PASS_MODE_GAUTHTIME:
        return validateGAuth(storedPassword.hash,passwordInput); // GAuth Time Based Token comparisson (seed,token)
    }

    switch (authMode)
    {
    case AUTH_MODE_PLAIN:
        return storedPassword.hash==toCompare? AUTH_REASON_AUTHENTICATED:AUTH_REASON_BAD_PASSWORD; // 1-1 comparisson
    case AUTH_MODE_CRAM:
        return saltedHash?AUTH_REASON_INTERNAL_ERROR:validateCRAM(storedPassword.hash, passwordInput, cramSalt);
    }

    return AUTH_REASON_NOT_IMPLEMENTED;
}

AuthReason IAuth_Validation_Account::validateCRAM(const std::string &passwordFromDB, const std::string &passwordInput, const std::string &cramSalt)
{
    // TODO:
    return AUTH_REASON_NOT_IMPLEMENTED;
}

AuthReason IAuth_Validation_Account::validateGAuth(const std::string &seed, const std::string &token)
{
    // TODO: (liboath)
    return AUTH_REASON_NOT_IMPLEMENTED;
}
