#ifndef IAUTH_PASSWORDDATA_H
#define IAUTH_PASSWORDDATA_H

#include <limits>
#include <string>
#include <map>

#include <string.h>
#include <cx2_hlp_functions/encoders.h>

#include "ds_auth_passmodes.h"

namespace CX2 { namespace Authorization { namespace DataStructs {


struct sPasswordBasicData
{
    sPasswordBasicData()
    {
        expiration = std::numeric_limits<time_t>::max();
        forceExpiration = false;
        passwordMode = PASS_MODE_PLAIN;
        memset(ssalt,0,4);
    }

    bool isPasswordExpired()
    {
        return expiration<time(nullptr);
    }

    PasswordModes passwordMode;
    unsigned char ssalt[4];
    time_t expiration;
    bool forceExpiration;

    char align[7];
};


struct sPasswordData
{
    sPasswordData()
    {
        gAuthSteps = 0; // means current.
        forceExpiration = false;
        passwordMode = PASS_MODE_PLAIN;
        expiration = std::numeric_limits<time_t>::max();
        memset(ssalt,0xFF,4);
    }

    sPasswordBasicData getBasicData()
    {
        sPasswordBasicData B;
        B.expiration = expiration;
        B.forceExpiration = forceExpiration;
        B.passwordMode = passwordMode;
        memcpy(B.ssalt, ssalt ,4);
        return B;
    }

    std::map<std::string,std::string> getMap() const
    {
        std::map<std::string,std::string> r;
        r["VERSION"] = "1";
        r["PMODE"] = std::to_string(passwordMode);
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
        if (mp.find(k)==mp.end()) return "";
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
        case 500: passwordMode = PASS_MODE_NOTFOUND;
            break;
        case 0: passwordMode = PASS_MODE_PLAIN;
            break;
        case 1: passwordMode = PASS_MODE_SHA256;
            break;
        case 2: passwordMode = PASS_MODE_SHA512;
            break;
        case 3: passwordMode = PASS_MODE_SSHA256;
            break;
        case 4: passwordMode = PASS_MODE_SSHA512;
            break;
        case 5: passwordMode = PASS_MODE_GAUTHTIME;
            break;
        default: passwordMode = PASS_MODE_NOTFOUND;
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

    bool isPasswordExpired() const
    {
        return expiration<time(nullptr) || forceExpiration;
    }

    uint32_t gAuthSteps;
    bool forceExpiration;
    PasswordModes passwordMode;
    time_t expiration;
    std::string hash;
    unsigned char ssalt[4];
};

}}}
#endif // IAUTH_PASSWORDDATA_H

