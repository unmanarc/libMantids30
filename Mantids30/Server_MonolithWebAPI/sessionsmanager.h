#pragma once

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Sessions/session.h>
#include <Mantids30/Threads/map.h>
#include <Mantids30/Threads/garbagecollector.h>
#include <Mantids30/Helpers/random.h>
#include <memory>
#include <mutex>

namespace Mantids30 { namespace Network { namespace Servers { namespace WebMonolith {


class WebSession : public Threads::Safe::MapItem
{
public:

    WebSession(std::shared_ptr<Mantids30::Sessions::Session> authSession, const json &newNetworkClientInfo)
    {
        this->networkClientInfo = newNetworkClientInfo;
        this->authSession = authSession;
    }

    bool compareUserAgent( const std::string & userAgent )
    {
        return JSON_ASSTRING(networkClientInfo,"userAgent","") == userAgent;
    }

    bool compareRemoteAddress( const std::string & remoteAddress )
    {
        return JSON_ASSTRING(networkClientInfo,"remoteAddress","") == remoteAddress;
    }

    std::shared_ptr<Mantids30::Sessions::Session> getAuthSession()
    {
        return authSession;
    }

    json getNetworkClientInfo()
    {
        return networkClientInfo;
    }


private:
    std::shared_ptr<Mantids30::Sessions::Session> authSession;
    json networkClientInfo;
    //std::string sessionId;
};

class WebSessionsManager : public Threads::GarbageCollector
{
public:
    WebSessionsManager();

    static void threadGC(void * sessManager);
    void gc();

    /**
     * @brief Retrieves the garbage collection (GC) wait time.
     *
     * This function returns the current wait time (in milliseconds) for the garbage collection process.
     *
     * @return The GC wait time in milliseconds.
     */
    uint32_t getGcWaitTime() const;
    /**
     * @brief Sets the garbage collection (GC) wait time.
     *
     * This function sets the wait time (in milliseconds) for the garbage collection process. The wait time
     * defines the delay before the GC process is triggered.
     *
     * @param waitTime The desired GC wait time in milliseconds.
     */
    void setGcWaitTime(const uint32_t &value);

    uint32_t getMaxInactiveSeconds() const;
    void setMaxInactiveSeconds(const uint32_t &value);

    uint32_t getMaxSessionsPerUser() const;
    void setMaxSessionsPerUser(const uint32_t &value);


    //void impersonateSession(const std::string & sessionID,std::shared_ptr<Sessions::Session> impersonatedSession);

    /**
     * @brief createSession Create a new web session
     * @param session Session element to be introduced to the session pool
     *                The session object will be deleted when the session is destroyed.
     * @return Session ID
     */
    std::string createSession(std::shared_ptr<Sessions::Session> session, const json & sessionInfo, uint64_t *maxAge);
    /**
     * @brief destroySession destroy the session element and session ID
     * @param sessionID Session ID String
     * @return true if destroyed, false otherwise (maybe there is no session with this ID)
     */
    bool destroySession(const std::string & sessionID);
    /**
     * @brief openSession Get the Web Session element and adquire a read lock on it.
     * @param sessionID Session ID
     * @param maxAge time left for session to expire
     * @return pointer to the web session.
     */
    WebSession *openSession(const std::string & sessionID, uint64_t *maxAge);
    /**
     * @brief releaseSession Release the session lock
     * @param sessionID
     * @return true if the session reader was released. false otherwise
     */
    bool releaseSession(const std::string & sessionID);
    /**
     * @brief Validates the format of a session ID.
     *
     * This function checks if the session ID meets the expected format:
     * 12 alphanumeric characters, followed by ":", and then another 12 alphanumeric characters.
     *
     * @param sessionID The session ID to validate.
     * @return true if the session ID matches the format; false otherwise.
     */
    static bool validateSessionIDFormat(const std::string& sessionID);
    /**
     * @brief Retrieves all active sessions for a given user from the web session manager.
     *
     * This function fetches the active sessions associated with the specified user name,
     * providing detailed information about each session in JSON format.
     *
     * @param effectiveUserName The username for which to retrieve session details.
     * @return A JSON object containing the user sessions, with session details as key-value pairs.
     */
    json getUserSessionsInfo(const std::string & effectiveUserName);

private:

    Threads::Safe::Map<std::string> m_sessions;

    std::mutex m_mutex;

    std::map<std::string, std::map<std::string, json> > m_sessionClientInfo;


    uint32_t m_gcWaitTime = 10;
    uint32_t m_MaxInactiveSeconds = 0;
    uint32_t m_maxSessionsPerUser = 0;
};


}}}}

