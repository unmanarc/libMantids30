#ifndef IAUTH_SESSION_H
#define IAUTH_SESSION_H

#include "data.h"
#include "domains.h"
#include "manager.h"
#include "session_vars.h"

#include <mutex>
#include <atomic>

namespace Mantids29 { namespace Authentication {


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
    enum eCheckMode
    {
        CHECK_ALLOW_EXPIRED_PASSWORDS,
        CHECK_DISALLOW_EXPIRED_PASSWORDS
    };

    Session(const std::string & appName);

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
     * @param accountName
     * @param accountDomain
     * @param passIndex
     * @param reason
     */
    void registerPersistentAuthentication(const std::string &accountName, const std::string &accountDomain, uint32_t passIndex, const Reason &reason);
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

    /**
     * @brief Sets the authenticated domain for the session.
     * @param domain The authenticated domain to set.
     */
    void setAuthenticatedDomain(const std::string& domain);

    /**
     * @brief Returns the authenticated domain for the session.
     * @return The authenticated domain for the session.
     */
    std::string getAuthenticatedDomain();

    /**
     * @brief Returns the application name associated with the authenticated session.
     * @return The application name associated with the authenticated session.
     */
    std::string getApplicationName();

    /**
     * @brief Sets the application name associated with the authenticated session.
     * @param name The application name to set.
     */
    void setApplicationName(const std::string& name);

    /**
     * @brief Sets the required basic authentication indices for the session.
     *
     * This method sets the required basic authentication indices for the session, which are used to authenticate the user.
     * @param authenticationIndices The required basic authentication indices to set.
     */
    void setRequiredBasicAuthenticationIndices(const std::map<uint32_t, std::string>& authenticationIndices);

    /**
     * @brief Checks if the session is fully authenticated.
     *
     * This method checks whether all required authenticated indices have been successfully authenticated for the session.
     * @param checkMode The check mode to use for the authentication.
     * @return true if the session is fully authenticated, false otherwise.
     */
    bool isFullyAuthenticated(const eCheckMode& checkMode);

    /**
     * @brief Returns the required authentication indices for the session.
     * @return A map of the required authentication indices for the session.
     */
    std::map<uint32_t, std::string> getRequiredAuthenticationIndices();

    /**
     * @brief Returns the next required authentication index for the session.
     * @return A pair containing the index and text value of the next required authentication index.
     */
    std::pair<uint32_t, std::string> getNextRequiredAuthenticationIndex();

    /**
     * @brief Checks whether the session is persistent.
     *
     * This method checks whether the session is persistent, i.e., whether it should persist across multiple application launches.
     * @return true if the session is persistent, false otherwise.
     */
    bool isPersistentSession();

    /**
     * @brief Sets whether the session is persistent.
     * @param value The value to set.
     */
    void setPersistentSession(bool value);


private:
    sCurrentAuthentication getCurrentAuthenticationStatus(const uint32_t &passIndex);
    /**
     * @brief getIdxAuthenticationStatus
     * @param passIndex
     * @return
     */
    Reason getIdxAuthenticationStatus_I(uint32_t passIndex = 0);

    void iUpdateLastActivity();


    std::string m_authenticatedUser;
    std::string m_authenticatedDomain;
    std::string m_applicationName;
    std::string m_sessionID;

    std::map<uint32_t,std::string> m_requiredLoginIndices;

    std::map<uint32_t,sCurrentAuthentication> m_authenticationMatrix;
    std::map<uint32_t,sAuthenticationPolicy> m_authenticationPolicies;

    time_t m_firstActivityTimestamp;
    std::atomic<time_t> m_lastActivityTimestamp;

    std::mutex m_authenticationMutex;

    bool m_isPersistentSession;
};

}}

#endif // IAUTH_SESSION_H
