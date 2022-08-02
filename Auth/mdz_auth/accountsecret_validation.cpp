#include "accountsecret_validation.h"

#include <mdz_hlp_functions/encoders.h>
#include <mdz_hlp_functions/crypto.h>

using namespace Mantids::Authentication;



AccountSecret_Validation::AccountSecret_Validation()
{
}

AccountSecret_Validation::~AccountSecret_Validation()
{

}



Reason AccountSecret_Validation::validateStoredSecret(const Secret &storedSecret, const std::string &passwordInput, const std::string &challengeSalt, Mode authMode)
{
    Reason r =REASON_NOT_IMPLEMENTED;
  //  bool saltedHash = false;
    std::string toCompare;

    switch (storedSecret.passwordFunction)
    {
    case FN_NOTFOUND:
        return REASON_INTERNAL_ERROR;
    case FN_PLAIN:
    {
        toCompare = passwordInput;
    } break;
    case FN_SHA256:
    {
        toCompare = Helpers::Crypto::calcSHA256(passwordInput);
    } break;
    case FN_SHA512:
    {
        toCompare = Helpers::Crypto::calcSHA512(passwordInput);
    } break;
    case FN_SSHA256:
    {
        toCompare = Helpers::Crypto::calcSSHA256(passwordInput, storedSecret.ssalt);
       // saltedHash = true;
    } break;
    case FN_SSHA512:
    {
        toCompare = Helpers::Crypto::calcSSHA512(passwordInput, storedSecret.ssalt);
        //saltedHash = true;
    } break;
    case FN_GAUTHTIME:
        r = validateGAuth(storedSecret.hash,passwordInput); // GAuth Time Based Token comparisson (seed,token)
        goto skipAuthMode;
    }

    switch (authMode)
    {
    case MODE_PLAIN:
        r = storedSecret.hash==toCompare? REASON_AUTHENTICATED:REASON_BAD_PASSWORD; // 1-1 comparisson
        break;
    case MODE_CHALLENGE:
        r = validateChallenge(storedSecret.hash, passwordInput, challengeSalt);
        break;
    }

skipAuthMode:;

    if (storedSecret.isExpired() && r==REASON_AUTHENTICATED)
        r = REASON_EXPIRED_PASSWORD;

    return r;
}

Reason AccountSecret_Validation::validateChallenge(const std::string &passwordFromDB, const std::string &challengeInput, const std::string &challengeSalt)
{
    return challengeInput == Helpers::Crypto::calcSHA256(passwordFromDB + challengeSalt) ?
                 REASON_AUTHENTICATED:REASON_BAD_PASSWORD;
}

Reason AccountSecret_Validation::validateGAuth(const std::string &seed, const std::string &token)
{
    // TODO: (liboath)
    return REASON_NOT_IMPLEMENTED;
}
