#ifndef IAUTH_PASSWORDDATA_H
#define IAUTH_PASSWORDDATA_H

#include <limits>
#include <string>
#include <map>

#include <string.h>
#include <time.h>

#include <cx2_hlp_functions/encoders.h>
#include <cx2_hlp_functions/crypto.h>
#include <cx2_hlp_functions/random.h>

#include "ds_auth_function.h"

namespace CX2 { namespace Authentication {


struct Secret_PublicData
{
    Secret_PublicData()
    {
        nul = true;
        expiration = 0;
        badAttempts = 0;
        forceExpiration = false;
        passwordFunction = FN_NOTFOUND;
        memset(ssalt,0,4);
        requiredAtLogin = false;
        locked = false;
    }

    bool isExpired() const
    {
        return (expiration<time(nullptr) && expiration!=0) || forceExpiration;
    }


    std::map<std::string,std::string> getMap() const
    {
        std::map<std::string,std::string> r;
        r["VERSION"] = "1";
        r["PMODE"] = std::to_string(passwordFunction);
        r["SALT"] = CX2::Helpers::Encoders::toHex(ssalt,4);
        r["EXPIRATION"] = std::to_string(expiration);
        r["FORCE_EXPIRATION"] = std::string(forceExpiration?"1":"0");
        r["BAD_ATTEMPTS"] = badAttempts;
        r["DESCRIPTION"] = description;
        r["REQUIRED_AT_LOGIN"] = std::string(requiredAtLogin?"1":"0");;
        r["LOCKED"] = locked;
        r["NUL"] = nul;
        return r;
    }

    std::string mget(std::map<std::string,std::string> mp, const std::string &k )
    {
        std::map<std::string,std::string> mpx;
        if ( mp.find(k)==mp.end() ) return "";
        return mp[k];
    }
    bool fromMap( const std::map<std::string,std::string> & mp )
    {
        if ( mget( mp, "VERSION" ) != "1" ) return false;

        CX2::Helpers::Encoders::fromHex(mget(mp,"SALT"),ssalt,4);
        expiration = strtoull(mget(mp,"EXPIRATION").c_str(), nullptr, 10);
        forceExpiration = strtoul(mget(mp,"FORCE_EXPIRATION").c_str(), nullptr, 10)?true:false;

        switch (strtoul(mget( mp, "PMODE" ).c_str(), nullptr, 10))
        {
        case 500: passwordFunction = FN_NOTFOUND;
            break;
        case 0: passwordFunction = FN_PLAIN;
            break;
        case 1: passwordFunction = FN_SHA256;
            break;
        case 2: passwordFunction = FN_SHA512;
            break;
        case 3: passwordFunction = FN_SSHA256;
            break;
        case 4: passwordFunction = FN_SSHA512;
            break;
        case 5: passwordFunction = FN_GAUTHTIME;
            break;
        default: passwordFunction = FN_NOTFOUND;
            break;
        }


        badAttempts = strtoul(mget(mp,"BAD_ATTEMPTS").c_str(), nullptr, 10);
        description = mget(mp,"DESCRIPTION");
        requiredAtLogin = strtoul(mget(mp,"REQUIRED_AT_LOGIN").c_str(), nullptr, 10)?true:false;
        locked = strtoul(mget(mp,"LOCKED").c_str(), nullptr, 10)?true:false;
        nul = strtoul(mget(mp,"NUL").c_str(), nullptr, 10)?true:false;

        return true;
    }


    Function passwordFunction;
    unsigned char ssalt[4];
    time_t expiration;
    bool forceExpiration;
    uint32_t badAttempts;
    std::string description;
    bool requiredAtLogin;
    bool locked;

    bool nul;
    char align[7];
};

struct Secret
{
    Secret()
    {
        badAttempts = 0;
        gAuthSteps = 0; // means current.
        forceExpiration = false;
        passwordFunction = FN_PLAIN;
        expiration = 0;
        memset(ssalt,0xFF,4);
    }

    Secret_PublicData getBasicData()
    {
        Secret_PublicData B;
        B.nul = false;
        B.locked = false;
        B.requiredAtLogin = false;
        B.badAttempts = badAttempts;
        B.expiration = expiration;
        B.forceExpiration = forceExpiration;
        B.passwordFunction = passwordFunction;
        memcpy(B.ssalt, ssalt ,4);
        return B;
    }

    std::map<std::string,std::string> getMap() const
    {
        std::map<std::string,std::string> r;
        r["VERSION"] = "1";
        r["PMODE"] = std::to_string(passwordFunction);
        r["HASH"] = hash;
        r["SALT"] = CX2::Helpers::Encoders::toHex(ssalt,4);
        r["EXPIRATION"] = std::to_string(expiration);
        r["FORCE_EXPIRATION"] = std::string(forceExpiration?"1":"0");
        r["GAUTH_STEPS"] = std::to_string(gAuthSteps);
        return r;
    }

    std::string mget(std::map<std::string,std::string> mp, const std::string &k )
    {
        std::map<std::string,std::string> mpx;
        if ( mp.find(k)==mp.end() ) return "";
        return mp[k];
    }

    bool fromMap( const std::map<std::string,std::string> & mp )
    {
        if ( mget( mp, "VERSION" ) != "1" ) return false;

        hash = mget(mp,"HASH");
        CX2::Helpers::Encoders::fromHex(mget(mp,"SALT"),ssalt,4);
        expiration = strtoull(mget(mp,"EXPIRATION").c_str(), nullptr, 10);
        forceExpiration = strtoul(mget(mp,"FORCE_EXPIRATION").c_str(), nullptr, 10)?true:false;
        gAuthSteps = strtoul(mget(mp,"GAUTH_STEPS").c_str(), nullptr, 10);

        switch (strtoul(mget( mp, "PMODE" ).c_str(), nullptr, 10))
        {
        case 500: passwordFunction = FN_NOTFOUND;
            break;
        case 0: passwordFunction = FN_PLAIN;
            break;
        case 1: passwordFunction = FN_SHA256;
            break;
        case 2: passwordFunction = FN_SHA512;
            break;
        case 3: passwordFunction = FN_SSHA256;
            break;
        case 4: passwordFunction = FN_SSHA512;
            break;
        case 5: passwordFunction = FN_GAUTHTIME;
            break;
        default: passwordFunction = FN_NOTFOUND;
            break;
        }

        return true;
    }

    time_t getExpiration() const
    {
        return expiration;
    }

    void setExpiration(const time_t &value)
    {
        expiration = value;
    }

    bool isExpired() const
    {
        return (time(nullptr)>expiration && expiration!=0) || forceExpiration;
    }


    uint32_t gAuthSteps;
    bool forceExpiration;
    Function passwordFunction;
    uint32_t badAttempts;
    time_t expiration;
    std::string hash;
    unsigned char ssalt[4];
};



static Secret createNewSecret(const std::string & passwordInput, const Function & passFunction, bool forceExpiration = false, const time_t &expirationDate = 0, uint32_t _2faSteps = 0)
{
    Secret r;

    r.passwordFunction = passFunction;
    r.forceExpiration = forceExpiration;
    r.expiration = expirationDate;

    switch (passFunction)
    {
    case FN_NOTFOUND:
    {
        // Do nothing...
    } break;
    case FN_PLAIN:
    {
        r.hash = passwordInput;
    } break;
    case FN_SHA256:
    {
        r.hash = Helpers::Crypto::calcSHA256(passwordInput);
    } break;
    case FN_SHA512:
    {
        r.hash = Helpers::Crypto::calcSHA512(passwordInput);
    } break;
    case FN_SSHA256:
    {
        CX2::Helpers::Random::createRandomSalt32(r.ssalt);
        r.hash = Helpers::Crypto::calcSSHA256(passwordInput, r.ssalt);
    } break;
    case FN_SSHA512:
    {
        CX2::Helpers::Random::createRandomSalt32(r.ssalt);
        r.hash = Helpers::Crypto::calcSSHA512(passwordInput, r.ssalt);
    } break;
    case FN_GAUTHTIME:
        r.hash = passwordInput;
        r.gAuthSteps = _2faSteps;
    }

    return r;
}


}}
#endif // IAUTH_PASSWORDDATA_H

