#include "session.h"
#include <Mantids30/Helpers/random.h>
#include <mutex>

using namespace Mantids30::Sessions;
using namespace Mantids30::DataFormat;

Session::Session(const JWT::Token &jwt)
    : m_firstActivityTimestamp(time(nullptr))
    , m_lastActivityTimestamp(time(nullptr))
{
    m_jwtAuthenticatedInfo = jwt;
    m_impersonator = jwt.getImpersonator();
    m_domain = jwt.getDomain();
    m_user = jwt.getSubject();
}

std::string Session::getUser()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_user;
}

void Session::updateLastActivity()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    iUpdateLastActivity();
}

bool Session::isLastActivityExpired(const uint32_t &expSeconds)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    time_t curTime = time(nullptr);
    if (m_lastActivityTimestamp > curTime)
    {
        return true; // Computer time has changed?
    }
    return (curTime - m_lastActivityTimestamp) > expSeconds;
}

time_t Session::getLastActivity()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    return m_lastActivityTimestamp;
}

void Session::iUpdateLastActivity()
{
    m_lastActivityTimestamp = time(nullptr);
}

std::string Session::getImpersonator()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_impersonator;
}

std::string Session::getDomain()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_domain;
}

JWT::Token Session::getJWTAuthenticatedInfo()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_jwtAuthenticatedInfo;
}

void Session::setLastActivity(const time_t &value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_lastActivityTimestamp = value;
}

time_t Session::getFirstActivity()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_firstActivityTimestamp;
}
