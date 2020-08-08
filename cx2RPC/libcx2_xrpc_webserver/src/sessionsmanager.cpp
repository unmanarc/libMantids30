#include "sessionsmanager.h"
#include <thread>
#include <cx2_hlp_functions/random.h>

using namespace CX2::RPC::Web;
using namespace CX2;

SessionsManager::SessionsManager()
{
    setGcWaitTime(1); // 1 sec.
    setSessionExpirationTime(600); // 10 min
    setMaxSessionsPerUser(100); // 100 sessions
}

SessionsManager::~SessionsManager()
{

}

void SessionsManager::gc()
{
    auto i = sessions.getKeys();
    for (const auto & key : i)
    {
        WebSession * s = (WebSession *)sessions.openElement(key);
        if (s && s->session->isLastActivityExpired(sessionExpirationTime))
        {
            sessions.closeElement( key );
            sessions.destroyElement( key );
        }
        else if (s)
        {
            sessions.closeElement( key );
        }

    }
}

void SessionsManager::threadGC(void *sessManager)
{
    SessionsManager * _sessManager = (SessionsManager *)sessManager;
    _sessManager->gc();
}

uint32_t SessionsManager::getSessionExpirationTime() const
{
    return sessionExpirationTime;
}

void SessionsManager::setSessionExpirationTime(const uint32_t &value)
{
    sessionExpirationTime = value;
}

std::string SessionsManager::addSession(Authorization::Session::IAuth_Session *session)
{
    auto userDomain = session->getUserDomainPair();
    {
        std::unique_lock<std::mutex> lock(mutex);

        if (sessionPerUser.find(userDomain) == sessionPerUser.end()) sessionPerUser[userDomain] = 1;
        else
        {
            if (sessionPerUser[userDomain] >= maxSessionsPerUser)
            {
                delete session;
                return "";
            }
            else sessionPerUser[userDomain]++;
        }
    }

    std::string sessionId = CX2::Helpers::Random::createRandomString(32);
    WebSession * xSession = new WebSession;
    session->setSessionId(sessionId);
    xSession->session = session;
    sessions.addElement(sessionId,xSession);
    return sessionId;
}

bool SessionsManager::destroySession(const std::string &sessionID)
{
    std::pair<std::string,std::string> userDomain;
    WebSession * sess;
    if ((sess=(WebSession *)sessions.openElement(sessionID))!=nullptr)
    {
        userDomain = sess->session->getUserDomainPair();
        sessions.closeElement(sessionID);
    }
    else return false;

    if (sessions.destroyElement(sessionID))
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (sessionPerUser.find(userDomain) == sessionPerUser.end())
        {
            throw std::runtime_error("Unregistered Session??");
        }
        else
        {
            sessionPerUser[userDomain]--;
            if (sessionPerUser[userDomain] == 0) sessionPerUser.erase(userDomain);
        }
        return true;
    }
    return false;
}

Authorization::Session::IAuth_Session *SessionsManager::openSession(const std::string &sessionID, uint64_t *maxAge)
{
    WebSession *xs;
    if ((xs=(WebSession *)sessions.openElement(sessionID))!=nullptr)
    {
        if (xs->session->isLastActivityExpired(sessionExpirationTime))
            *maxAge = 0;
        else
            *maxAge = (xs->session->getLastActivity()+sessionExpirationTime)-time(nullptr);
        return xs->session;
    }
    return nullptr;
}

bool SessionsManager::closeSession(const std::string &sessionID)
{
    return sessions.closeElement(sessionID);
}

uint32_t SessionsManager::getMaxSessionsPerUser() const
{
    return maxSessionsPerUser;
}

void SessionsManager::setMaxSessionsPerUser(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(mutex);
    maxSessionsPerUser = value;
}

uint32_t SessionsManager::getGcWaitTime() const
{
    return gcWaitTime;
}

void SessionsManager::setGcWaitTime(const uint32_t &value)
{
    gcWaitTime = value;
}
