#pragma once

#include "session_vars.h"
#include <Mantids30/Helpers/json.h>
#include <Mantids30/DataFormat_JWT/jwt.h>

#include <mutex>
#include <atomic>
#include <string>

namespace Mantids30 { namespace Sessions {
/*

struct SlotAuthenticationPolicy
{
    SlotAuthenticationPolicy()
    {
        validTime = 0;
    }

    time_t validTime;
};


struct CurrentAuthenticationStatus
{
    CurrentAuthenticationStatus()
    {
        lastAuthStatus = REASON_UNAUTHENTICATED;
    }
    void setCurrentTime()
    {
        authTime = time(nullptr);
    }

    Reason lastAuthStatus;
    time_t authTime;
};
*/

/**
 * @brief The ClientDetails struct contains details about an incoming client connection.
 */
struct ClientDetails {
    /**
     * @brief The IP address of the incoming client connection.
     */
    std::string ipAddress;

    /**
     * @brief Additional data associated with the incoming client connection.
     */
    std::string extraData;

    /**
     * @brief The common name associated with the client's TLS certificate.
     */
    std::string tlsCommonName;

    /**
     * @brief The user agent string associated with the incoming client connection.
     */
    std::string userAgent;
};

class Session : public Session_Vars
{
public:
    enum eCheckMode
    {
        CHECK_ALLOW_EXPIRED_PASSWORDS,
        CHECK_DISALLOW_EXPIRED_PASSWORDS
    };

    /**
     * @brief Session Initialize session class.
     */
    //Session();

    /**
     * @brief Session Initialize a session based on a JWT token
     *          Warning: you should verify that the token is valid previous to the initialization
     *          this function will just initialize all the data structures based on the token itselft, but
     *          discard signature validation.
     * @param jwt JWT Token
     */
    Session(const DataFormat::JWT::Token &jwt );

    /**
     * @brief updateLastActivity Set las activity to current time
     */
    void updateLastActivity();
    /**
     * @brief isLastActivityExpired given an expiration time in seconds, tell if this session is expired (inactive)
     * @param expSeconds
     * @return true if expired
     */
    bool isLastActivityExpired(const uint32_t & expSeconds);
    /**
     * @brief getLastActivity Get the unix time from the last activity
     * @return unix time of last activity
     */
    time_t getLastActivity();
    /**
     * @brief setLastActivity Set last activity time (for assign operations), use updateLastActivity for updating this value
     * @param value unix time
     */
    void setLastActivity(const time_t &value);
    /**
     * @brief getUser Get the user name
     * @return user id string (userName)
     */
    std::string getUser();
    /**
     * @brief setAuthenticatedUser Set/Change the authenticated user
     * @param value user that authenticated
     */
    //void setUser(const std::string &value);
    /**
     * @brief getFirstActivity Get the unix time from where you created this session
     * @return unix time
     */
    time_t getFirstActivity();

    // TODO: Validate that the session has not been revoked.
    bool isSessionRevoked() { return false; }

    DataFormat::JWT::Token getJWTAuthenticatedInfo();

//    void setJWTAuthenticatedInfo(const DataFormat::JWT::Token &newJwtAuthenticatedInfo);

    std::string getDomain();
    //void setDomain(const std::string &newDomain);

    std::string getImpersonator();
    //void setImpersonator(const std::string &newImpersonator);

private:
    void iUpdateLastActivity();

    DataFormat::JWT::Token jwtAuthenticatedInfo;

    std::string m_user, m_domain, m_impersonator;

    time_t m_firstActivityTimestamp = 0;
    std::atomic<time_t> m_lastActivityTimestamp;
    std::mutex m_authenticationMutex;
};

}}


