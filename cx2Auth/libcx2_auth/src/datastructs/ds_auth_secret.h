#ifndef IAUTH_PASSWORDDATA_H
#define IAUTH_PASSWORDDATA_H

#include <limits>
#include <string>
#include <map>

#include <string.h>
#include <time.h>

#include <cx2_hlp_functions/encoders.h>

#include "ds_auth_function.h"

namespace CX2 { namespace Authentication {


struct Secret_PublicData
{
    Secret_PublicData()
    {
        expiration = 0;
        forceExpiration = false;
        passwordFunction = FN_PLAIN;
        memset(ssalt,0,4);
    }

    bool isExpired()
    {
        return expiration<time(nullptr);
    }

    Function passwordFunction;
    unsigned char ssalt[4];
    time_t expiration;
    bool forceExpiration;

    char align[7];
};

struct Secret
{
    Secret()
    {
        gAuthSteps = 0; // means current.
        forceExpiration = false;
        passwordFunction = FN_PLAIN;
        expiration = 0;
        memset(ssalt,0xFF,4);
    }

    Secret_PublicData getBasicData()
    {
        Secret_PublicData B;
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
    time_t expiration;
    std::string hash;
    unsigned char ssalt[4];
};

}}
#endif // IAUTH_PASSWORDDATA_H

