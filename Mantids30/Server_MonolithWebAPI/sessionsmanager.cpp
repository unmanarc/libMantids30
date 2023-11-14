#include "sessionsmanager.h"
#include <Mantids30/Helpers/random.h>

#include <stdexcept>

using namespace Mantids30::Network::Servers::WebMonolith;
using namespace Mantids30;

SessionsManager::SessionsManager()
{
    setGcWaitTime(1); // 1 sec.
    setSessionExpirationTime(900); // 15 min
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
        if (s && s->authSession->isLastActivityExpired(sessionExpirationTime))
        {
            sessions.releaseElement( key );
            sessions.destroyElement( key );
        }
        else if (s)
        {
            sessions.releaseElement( key );
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

std::string SessionsManager::createWebSession(Mantids30::Auth::Session *session)
{
    std::string effectiveUser = session->getEffectiveUser();
    {
        std::unique_lock<std::mutex> lock(mutex);

        if (sessionPerUser.find(effectiveUser) == sessionPerUser.end()) sessionPerUser[userDomain] = 1;
        else
        {
            if (sessionPerUser[effectiveUser] >= maxSessionsPerUser)
            {
                return "";
            }
            else sessionPerUser[effectiveUser]++;
        }
    }

    std::string sessionId = Mantids30::Helpers::Random::createRandomString(12) + ":" + Mantids30::Helpers::Random::createRandomString(12);
    WebSession * webSession = new WebSession;
    session->setSessionId(sessionId);
    webSession->authSession = session;
    if (!sessions.addElement(sessionId,webSession))
    {
        delete webSession;
        return "";
    }
    return sessionId;
}

bool SessionsManager::destroySession(const std::string &sessionID)
{
    std::string effectiveUser;
    WebSession * sess;
    if ((sess=(WebSession *)sessions.openElement(sessionID))!=nullptr)
    {
        effectiveUser = sess->authSession->getEffectiveUser();
        sessions.releaseElement(sessionID);
    }
    else return false;

    if (sessions.destroyElement(sessionID))
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (sessionPerUser.find(effectiveUser) == sessionPerUser.end())
        {
            throw std::runtime_error("Unregistered Session??");
        }
        else
        {
            sessionPerUser[effectiveUser]--;
            if (sessionPerUser[effectiveUser] == 0) sessionPerUser.erase(effectiveUser);
        }
        return true;
    }
    return false;
}

WebSession *SessionsManager::openSession(const std::string &sessionID, uint64_t *maxAge)
{
    WebSession *xs;
    if ((xs=(WebSession *)sessions.openElement(sessionID))!=nullptr)
    {
        if (xs->authSession->isLastActivityExpired(sessionExpirationTime))
            *maxAge = 0;
        else
            *maxAge = (xs->authSession->getLastActivity()+sessionExpirationTime)-time(nullptr);
        return xs;
    }
    return nullptr;
}

bool SessionsManager::releaseSession(const std::string &sessionID)
{
    return sessions.releaseElement(sessionID);
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
