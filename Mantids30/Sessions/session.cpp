#include "session.h"
#include <Mantids30/Helpers/random.h>
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Sessions;
using namespace Mantids30::DataFormat;

Session::Session(const JWT::Token &jwt)  : m_firstActivityTimestamp(time(nullptr)), m_lastActivityTimestamp(time(nullptr))
    //: Session() // Delegates to the default constructor
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
    if (m_lastActivityTimestamp>curTime) return true; // Computer time has changed?
    return (uint32_t)(curTime-m_lastActivityTimestamp)>expSeconds;
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
/*
void Session::setImpersonator(const std::string &newImpersonator)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_impersonator = newImpersonator;
}*/

std::string Session::getDomain()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_domain;
}
/*
void Session::setDomain(const std::string &newDomain)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_domain = newDomain;
}

void Session::setJWTAuthenticatedInfo(const JWT::Token &newJwtAuthenticatedInfo)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    jwtAuthenticatedInfo = newJwtAuthenticatedInfo;
}
*/
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

/*void Session::setUser(const std::string &value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_user = value;
}
*/
time_t Session::getFirstActivity()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_firstActivityTimestamp;
}
