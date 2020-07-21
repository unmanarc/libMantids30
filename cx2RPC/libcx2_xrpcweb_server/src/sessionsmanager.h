#ifndef XRPC_SESSIONS_MANAGER_H
#define XRPC_SESSIONS_MANAGER_H

#include <cx2_auth/iauth_session.h>
#include <cx2_thr_mutex_map/map.h>
#include <cx2_thr_threads/garbagecollector.h>

namespace CX2 { namespace RPC { namespace XRPCWeb {

class WebSession : public Threads::Safe::Map_Element
{
public:
    WebSession() {}
    ~WebSession() { delete session; }
    Authorization::Session::IAuth_Session * session;
};

class SessionsManager : public Threads::GarbageCollector
{
public:
    SessionsManager();
    ~SessionsManager();

    static void threadGC(void * sessManager);
    void gc();

    uint32_t getGcWaitTime() const;
    void setGcWaitTime(const uint32_t &value);

    uint32_t getSessionExpirationTime() const;
    void setSessionExpirationTime(const uint32_t &value);

    std::string addSession(Authorization::Session::IAuth_Session * session);
    bool destroySession(const std::string & sessionID);
    Authorization::Session::IAuth_Session * openSession(const std::string & sessionID);
    bool closeSession(const std::string & sessionID);

    uint32_t getMaxSessionsPerUser() const;
    void setMaxSessionsPerUser(const uint32_t &value);

private:
    std::map<std::pair<std::string,std::string>,uint32_t> sessionPerUser;
    std::mutex mutex;

    Threads::Safe::Map<std::string> sessions;
    uint32_t gcWaitTime;
    uint32_t sessionExpirationTime;
    uint32_t maxSessionsPerUser;
};


}}}

#endif // XRPC_SESSIONS_MANAGER_H
