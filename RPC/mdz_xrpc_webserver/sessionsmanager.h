#ifndef XRPC_SESSIONS_MANAGER_H
#define XRPC_SESSIONS_MANAGER_H

#include <mdz_auth/session.h>
#include <mdz_thr_safecontainers/map.h>
#include <mdz_thr_threads/garbagecollector.h>
#include <mdz_hlp_functions/random.h>

namespace Mantids { namespace RPC { namespace Web {

class WebSession : public Threads::Safe::Map_Element
{
public:
    WebSession()
    {
        authSession = nullptr;
        bAuthTokenConfirmed = false;
        sCSRFAuthConfirmToken = Mantids::Helpers::Random::createRandomString(32);
        sCSRFToken = Mantids::Helpers::Random::createRandomString(32);
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

    Mantids::Authentication::Session * authSession;
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

    std::string createWebSession(Mantids::Authentication::Session * session);
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
