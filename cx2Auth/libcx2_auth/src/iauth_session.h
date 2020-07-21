#ifndef IAUTH_SESSION_H
#define IAUTH_SESSION_H

#include <string>
#include <map>
#include <mutex>
#include <atomic>

#include "iauth.h"

namespace CX2 { namespace Authorization { namespace Session {

struct sAuthenticationPolicy
{
    sAuthenticationPolicy()
    {
        validTime = 0;
    }

    time_t validTime;
};


struct sCurrentAuthentication
{
    sCurrentAuthentication()
    {
        lastAuthReason = DataStructs::AUTH_REASON_UNAUTHENTICATED;
    }
    void setCurrentTime()
    {
        authTime = time(nullptr);
    }

    DataStructs::AuthReason lastAuthReason;
    time_t authTime;
};


class IAuth_Session
{
public:
    IAuth_Session();
    IAuth_Session& operator=(IAuth_Session &value)
    {
        this->authUser = value.authUser;

        this->sessionId = value.sessionId;
        this->authMatrix = value.authMatrix;
        this->authPolicies = value.authPolicies;
        this->firstActivity = value.firstActivity;

        return *this;
    }

    /**
     * @brief getAuthUser
     * @return
     */
    std::string getAuthUser();
    /**
     * @brief setIndexAuthenticationPolicy
     * @param passIndex
     * @param authPolicy
     */
    void setIndexAuthenticationPolicy(uint32_t passIndex, const sAuthenticationPolicy & authPolicy);
    /**
     * @brief isAuthenticated
     * @param passIndex
     * @return
     */
    DataStructs::AuthReason isAuthenticated(uint32_t passIndex = 0);
    /**
     * @brief authenticate Validate account password
     * @param accountName
     * @param password
     * @param passIndex
     * @param authMode
     * @param cramSalt
     * @return
     */
    void registerPersistentAuthentication(const std::string &accountName, const std::string &accountDomain, uint32_t passIndex, const DataStructs::AuthReason &reason);
    /**
     * @brief updateLastActivity Set las activity to current time
     */
    void updateLastActivity();
    /**
     * @brief isLastActivityExpired given an expiration time in seconds, tell if this session is expired (inactive)
     * @param expSeconds
     * @return true if expired
     */
    bool isLastActivityExpired(const uint32_t & expSeconds) const;
    /**
     * @brief getLastActivity Get the unix time from the last activity
     * @return unix time of last activity
     */
    time_t getLastActivity() const;
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
    void regenSessionId();
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
     * @brief getUserDomainPair Get User and Domain in string pair
     * @return
     */
    std::pair<std::string,std::string> getUserDomainPair() const;
    /**
     * @brief getUserID Get the user ID
     * @return user id string (username)
     */
    std::string getUserID();
    /**
     * @brief setAuthUser Set/Change the authenticated user
     * @param value user that authenticated
     */
    void setAuthUser(const std::string &value);
    /**
     * @brief getFirstActivity Get the unix time from where you created this session
     * @return unix time
     */
    time_t getFirstActivity();

    void setAuthDomain(const std::string &value);

    std::string getAuthDomain();

private:
    sCurrentAuthentication getCurrentAuthentication(const uint32_t &passIndex);

    std::string authUser, authDomain;
    std::string sessionId;
    std::map<uint32_t,sCurrentAuthentication> authMatrix;
    std::map<uint32_t,sAuthenticationPolicy> authPolicies;

    time_t firstActivity;
    std::atomic<time_t> lastActivity;

    std::mutex mutexAuth;
};

}}}

#endif // IAUTH_SESSION_H
