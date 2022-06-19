#ifndef IAUTH_SESSION_H
#define IAUTH_SESSION_H

#include "manager.h"
#include "session_vars.h"

#include <mutex>
#include <atomic>

namespace Mantids { namespace Authentication {

enum eCheckMode
{
    CHECK_ALLOW_EXPIRED_PASSWORDS,
    CHECK_DISALLOW_EXPIRED_PASSWORDS
};

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
    Session(const std::string & appName);
   /* Session& operator=(Session &value)
    {
        this->appName = value.appName;
        this->authUser = value.authUser;

        this->sessionId = value.sessionId;
        this->authMatrix = value.authMatrix;
        this->authPolicies = value.authPolicies;
        this->firstActivity = value.firstActivity;

        return *this;
    }*/
    /**
     * @brief getIdxAuthenticationStatus
     * @param passIndex
     * @return
     */
    Reason getIdxAuthenticationStatus(uint32_t passIndex = 0);
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
     * @brief registerPersistentAuthentication
     * @param sAccountName
     * @param accountDomain
     * @param passIndex
     * @param reason
     */
    void registerPersistentAuthentication(const std::string &sAccountName, const std::string &accountDomain, uint32_t passIndex, const Reason &reason);
    /**
     * @brief registerPersistentAuthentication
     * @param passIndex
     * @param reason
     */
    void registerPersistentAuthentication(uint32_t passIndex, const Reason &reason);
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
     * @return user id string (userName)
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

    std::string getAppName();
    void setAppName(const std::string &value);

    void setRequiredLoginIdx(const std::map<uint32_t, std::string> &value);
    bool getIsFullyLoggedIn(const eCheckMode & eCheckMode);
    std::map<uint32_t,std::string> getRequiredLoginIdxs();
    std::pair<uint32_t, std::string> getNextRequiredLoginIdxs();

    bool getIsPersistentSession();
    void setIsPersistentSession(bool value);

private:
    sCurrentAuthentication getCurrentAuthenticationStatus(const uint32_t &passIndex);
    /**
     * @brief getIdxAuthenticationStatus
     * @param passIndex
     * @return
     */
    Reason getIdxAuthenticationStatus_I(uint32_t passIndex = 0);

    void iUpdateLastActivity();

    std::string authUser, authDomain, appName;
    std::string sessionId;

    std::map<uint32_t,std::string> requiredLoginIdxs;

    std::map<uint32_t,sCurrentAuthentication> authMatrix;
    std::map<uint32_t,sAuthenticationPolicy> authPolicies;

    time_t firstActivity;
    std::atomic<time_t> lastActivity;

    std::mutex mutexAuth;

    bool isPersistentSession;

};

}}

#endif // IAUTH_SESSION_H
