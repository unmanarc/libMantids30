#include "sessionsmanager.h"
#include <Mantids30/Helpers/random.h>
#include <Mantids30/Program_Logs/rpclog.h>

#include <memory>
#include <stdexcept>
#include <regex>

using namespace Mantids30::Network::Servers::WebMonolith;
using namespace Mantids30;

WebSessionsManager::WebSessionsManager()
{
    setGcWaitTime(1); // 1 sec.
    setMaxInactiveSeconds(900); // 15 min
    setMaxSessionsPerUser(100); // 100 sessions
}

WebSessionsManager::~WebSessionsManager()
{
}

void WebSessionsManager::gc()
{
    auto i = m_sessions.getKeys();
    for (const auto & key : i)
    {
        WebSession * s = (WebSession *)m_sessions.openElement(key);
        if (s && s->getAuthSession()->isLastActivityExpired(m_MaxInactiveSeconds))
        {
            m_sessions.releaseElement( key );
            m_sessions.destroyElement( key );
            // TODO: log?
        }
        else if (s)
        {
            m_sessions.releaseElement( key );
        }
    }
}

void WebSessionsManager::threadGC(void *sessManager)
{
    WebSessionsManager * _sessManager = (WebSessionsManager *)sessManager;
    _sessManager->gc();
}

uint32_t WebSessionsManager::getMaxInactiveSeconds() const
{
    return m_MaxInactiveSeconds;
}

void WebSessionsManager::setMaxInactiveSeconds(const uint32_t &value)
{
    m_MaxInactiveSeconds = value;
}

std::string WebSessionsManager::createSession(std::shared_ptr<Mantids30::Sessions::Session> session, const json & networkClientInfo, uint64_t *maxAge)
{
    std::string sessionId = Mantids30::Helpers::Random::createRandomString(12) + ":" + Mantids30::Helpers::Random::createRandomString(12);

    *maxAge = 0;
    std::string effectiveUser = session->getUser();
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_sessionClientInfo.find(effectiveUser) == m_sessionClientInfo.end())
        {
            m_sessionClientInfo[effectiveUser][sessionId] = networkClientInfo;
        }
        else
        {
            if (m_sessionClientInfo[effectiveUser].size() >= m_maxSessionsPerUser)
            {
                // Max sessions per user reached.
                return "";
            }
            else
            {
                m_sessionClientInfo[effectiveUser][sessionId] = networkClientInfo;
            }
        }
    }

    WebSession * webSession = new WebSession(session,networkClientInfo);
    // New session, full time.
    *maxAge = m_MaxInactiveSeconds;

    if (!m_sessions.addElement(sessionId,webSession))
    {
        // Session ID Already exist... (far too rare condition)
        delete webSession;
        return "";
    }

    return sessionId;
}

bool WebSessionsManager::destroySession(const std::string &sessionID)
{
    std::string effectiveUser;
    WebSession * sess;
    if ((sess=(WebSession *)m_sessions.openElement(sessionID))!=nullptr)
    {
        // Race condition?
        effectiveUser = sess->getAuthSession()->getUser();
        m_sessions.releaseElement(sessionID);
    }
    else return false;

    if (m_sessions.destroyElement(sessionID))
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (
            m_sessionClientInfo.find(effectiveUser) == m_sessionClientInfo.end()
            || m_sessionClientInfo[effectiveUser].find(sessionID) == m_sessionClientInfo[effectiveUser].end()
            )
        {
            throw std::runtime_error("Unregistered Session??");
        }
        else
        {
            m_sessionClientInfo[effectiveUser].erase(sessionID);
            if (m_sessionClientInfo[effectiveUser].empty())
            {
                m_sessionClientInfo.erase(effectiveUser);
            }
        }
        return true;
    }
    return false;
}

WebSession *WebSessionsManager::openSession(const std::string &sessionID, uint64_t *maxAge)
{
    WebSession *xs;
    if ((xs=(WebSession *)m_sessions.openElement(sessionID))!=nullptr)
    {
        uint64_t lastActivity = xs->getAuthSession()->getLastActivity();

        if (xs->getAuthSession()->isLastActivityExpired(m_MaxInactiveSeconds))
        {
            *maxAge = 0;
        }
        else
        {
            uint64_t expirationTime = lastActivity + m_MaxInactiveSeconds;
            uint64_t currentTime = static_cast<uint64_t>(time(nullptr));

            // Ensure no underflow
            *maxAge = (expirationTime > currentTime) ? (expirationTime - currentTime) : 0;
        }
        return xs;
    }
    return nullptr;
}

bool WebSessionsManager::releaseSession(const std::string &sessionID)
{
    return m_sessions.releaseElement(sessionID);
}

bool WebSessionsManager::validateSessionIDFormat(const std::string &sessionID)
{
    if (sessionID.empty())
        return true;

    // Regular expression for 12 alphanumeric characters, followed by ":", and another 12 alphanumeric characters.
    static const std::regex pattern("^[a-zA-Z0-9]{12}:[a-zA-Z0-9]{12}$");
    return std::regex_match(sessionID, pattern);
}

json WebSessionsManager::getUserSessionsInfo(const std::string &effectiveUserName)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    json r;
    auto sessionsMap = m_sessionClientInfo[effectiveUserName];

    for ( const auto & sessions : sessionsMap )
    {
        // provide truncated session information.
        r[Program::Logs::RPCLog::truncateSessionId(sessions.first)] =  sessions.second;
    }

    return r;
}

uint32_t WebSessionsManager::getMaxSessionsPerUser() const
{
    return m_maxSessionsPerUser;
}

void WebSessionsManager::setMaxSessionsPerUser(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_maxSessionsPerUser = value;
}
/*
void WebSessionsManager::impersonateSession(const std::string &sessionID, std::shared_ptr<Sessions::Session> impersonatedSession)
{
    uint64_t maxAge;
    WebSession *session = openSession(sessionID, &maxAge);
    if (session)
    {
        session->setImpersonatedAuthSession(impersonatedSession);
        releaseSession(sessionID);
    }
}*/

uint32_t WebSessionsManager::getGcWaitTime() const
{
    return m_gcWaitTime;
}

void WebSessionsManager::setGcWaitTime(const uint32_t &value)
{
    m_gcWaitTime = value;
}

