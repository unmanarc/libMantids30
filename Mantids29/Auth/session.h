#pragma once

#include "ds_authentication.h"
#include "session_vars.h"

#include <Mantids29/Helpers/json.h>

#include <mutex>
#include <atomic>
#include <string>

namespace Mantids29 { namespace Auth {


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


class Session : public Session_Vars
{
public:
    enum eCheckMode
    {
        CHECK_ALLOW_EXPIRED_PASSWORDS,
        CHECK_DISALLOW_EXPIRED_PASSWORDS
    };

    Session();

    /**
     * @brief getEffectiveUser Get effective user (if impersonated, use the impersonated user...)
     * @return effective user
     */
    std::string getEffectiveUser();

    /**
     * @brief getAuthUser
     * @return
     */
    //std::string getAuthUser();
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
     * @brief createNewSessionID Create New Session ID String from RANDOM (does not modify current)
     * @return session id string
     */
    std::string createNewSessionIDStr();
    /**
     * @brief regenSessionId regenerate the session id using a pseudo-random number
     */
    void regenerateSessionID();
    /**
     * @brief setSessionId Set the session id string
     * @param value session id
     */
    void setSessionId(const std::string &value);
    /**
     * @brief getSessionId Get the current session ID
     * @return session ID string (in hex value)
     */
    std::string getSessionId();
    /**
     * @brief getUserID Get the user ID
     * @return user id string (userName)
     */
    std::string getUserID();
    /**
     * @brief setAuthenticatedUser Set/Change the authenticated user
     * @param value user that authenticated
     */
    void setAuthenticatedUser(const std::string &value);
    /**
     * @brief getFirstActivity Get the unix time from where you created this session
     * @return unix time
     */
    time_t getFirstActivity();
    /**
     * @brief Returns the application name associated with the authenticated session.
     * @return The application name associated with the authenticated session.
     */
   // std::string getApplicationName();
    /**
     * @brief Sets the application name associated with the authenticated session.
     * @param name The application name to set.
     */
    //void setApplicationName(const std::string& name);
    /**
     * @brief Checks whether the session is persistent.
     *
     * This method checks whether the session is persistent, i.e., whether it should persist across multiple application launches.
     * @return true if the session is persistent, false otherwise.
     */
    //bool isPersistentSession();
    /**
     * @brief Sets whether the session is persistent.
     * @param value The value to set.
     */
    //void setIsPersistentSession(bool value);

    std::map<std::string, json> getClaims();
    json getClaim(const std::string &claimName);
    void setClaims(const std::map<std::string, json> &newClaims);
    void addClaims(const std::map<std::string, json> &newClaims);

    bool validateAppPermissionInClaim(const std::string &permissionName);
    bool isAdmin();

    std::set<std::string> getActivities();

    // TODO: validate que la sesión no haya sido revocada..
    bool validateSession() { return true; }


    //std::string impersonatedUser();
    void setImpersonatedUser(const std::string &newImpersonatedUser);

private:
    void iUpdateLastActivity();

    std::map<std::string,json> m_claims;

    std::string m_authenticatedUser, m_impersonatedUser;
    std::string m_sessionID;
    //std::string m_applicationName;

    time_t m_firstActivityTimestamp = 0;
    std::atomic<time_t> m_lastActivityTimestamp;
    std::mutex m_authenticationMutex;

    //bool m_isPersistentSession;
};

}}


