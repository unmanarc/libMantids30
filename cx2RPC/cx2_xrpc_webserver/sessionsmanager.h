#ifndef XRPC_SESSIONS_MANAGER_H
#define XRPC_SESSIONS_MANAGER_H

#include <cx2_auth/session.h>
#include <cx2_thr_safecontainers/map.h>
#include <cx2_thr_threads/garbagecollector.h>
#include <cx2_hlp_functions/random.h>

namespace CX2 { namespace RPC { namespace Web {

class WebSession : public Threads::Safe::Map_Element
{
public:
    WebSession()
    {
        authSession = nullptr;
        bAuthTokenConfirmed = false;
        sCSRFAuthConfirmToken = CX2::Helpers::Random::createRandomString(32);
        sCSRFToken = CX2::Helpers::Random::createRandomString(32);
    }
    ~WebSession() { delete authSession; }

    bool validateCSRFToken(const std::string & token)
    {
        return token == sCSRFToken;
    }
    bool confirmAuthCSRFToken(const std::string & token)
    {
        bAuthTokenConfirmed = (token == sCSRFAuthConfirmToken);
        return bAuthTokenConfirmed;
    }

    CX2::Authentication::Session * authSession;
    std::string sCSRFAuthConfirmToken, sCSRFToken;
    std::atomic<bool> bAuthTokenConfirmed;
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

    uint32_t getMaxSessionsPerUser() const;
    void setMaxSessionsPerUser(const uint32_t &value);

    std::string createWebSession(CX2::Authentication::Session * session);
    bool destroySession(const std::string & sessionID);
    WebSession *openSession(const std::string & sessionID, uint64_t *maxAge);
    bool closeSession(const std::string & sessionID);


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
