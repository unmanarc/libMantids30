#include "session.h"
#include <iomanip>
#include <sstream>
#include <Mantids29/Helpers/random.h>
#include <Mantids29/Threads/lock_shared.h>

using namespace Mantids29::Authentication;

Session::Session(const std::string &appName)
{
    this->m_isPersistentSession = false;
    this->m_applicationName = appName;
    regenerateSessionID();
}

Reason Session::getIdxAuthenticationStatus(uint32_t passIndex)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    return getIdxAuthenticationStatus_I(passIndex);
}

void Session::registerPersistentAuthentication(const std::string & accountName, const std::string &accountDomain, uint32_t passIndex, const Reason & reason)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    m_authenticationMatrix[passIndex].lastAuthStatus = reason;
    m_authenticationMatrix[passIndex].setCurrentTime();

    if ( IS_PASSWORD_AUTHENTICATED(reason) )
    {
        iUpdateLastActivity();
        m_firstActivityTimestamp = m_lastActivityTimestamp;
    }
    // Authenticated with the main password:
    if (!passIndex && IS_PASSWORD_AUTHENTICATED( reason ) )
    {
        m_authenticatedUser = accountName;
        m_authenticatedDomain = accountDomain;
    }
}

void Session::registerPersistentAuthentication(uint32_t passIndex, const Reason &reason)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    m_authenticationMatrix[passIndex].lastAuthStatus = reason;
    m_authenticationMatrix[passIndex].setCurrentTime();

    if ( IS_PASSWORD_AUTHENTICATED(reason) )
    {
        iUpdateLastActivity();
        m_firstActivityTimestamp = m_lastActivityTimestamp;
    }
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

void Session::regenerateSessionID()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    m_sessionID = createNewSessionIDStr();
}

std::string Session::getUserID()
{
    return m_authenticatedUser;
}

sCurrentAuthentication Session::getCurrentAuthenticationStatus(const uint32_t &passIndex)
{
   // std::unique_lock<std::mutex> lock(mutexAuth); < private method.
    if (m_authenticationMatrix.find(passIndex)!=m_authenticationMatrix.end())
        return m_authenticationMatrix[passIndex];
    sCurrentAuthentication x;
    return x;
}

Reason Session::getIdxAuthenticationStatus_I(uint32_t passIndex)
{
    sCurrentAuthentication curAuthStatus;

    curAuthStatus = getCurrentAuthenticationStatus(passIndex);
    if ( IS_PASSWORD_AUTHENTICATED( curAuthStatus.lastAuthStatus ) )
    {
        // If authenticated: check if exist a policy for that idx
        if (m_authenticationPolicies.find(passIndex)!=m_authenticationPolicies.end())
        {
            // Then, if exist, check the max auth time.
            if (m_authenticationPolicies[passIndex].validTime>0 && curAuthStatus.authTime+m_authenticationPolicies[passIndex].validTime < time(nullptr))
                return REASON_EXPIRED;
        }
    }
    return curAuthStatus.lastAuthStatus;
}

void Session::iUpdateLastActivity()
{
    m_lastActivityTimestamp = time(nullptr);

}

bool Session::isPersistentSession()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    return m_isPersistentSession;
}

void Session::setPersistentSession(bool value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    m_isPersistentSession = value;
}

std::map<uint32_t, std::string> Session::getRequiredAuthenticationIndices()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    return m_requiredLoginIndices;
}

std::pair<uint32_t, std::string> Session::getNextRequiredAuthenticationIndex()
{
    std::pair<uint32_t, std::string> r = std::make_pair(0xFFFFFFFF,"");
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    for (const auto & i : m_requiredLoginIndices)
    {
        // Get the first required password that is not authenticated (allow expired and valid pass):
        if ( !IS_PASSWORD_AUTHENTICATED(getIdxAuthenticationStatus_I(i.first)) )
            return i;
    }

    return r;
}

void Session::setRequiredBasicAuthenticationIndices(const std::map<uint32_t,std::string> &value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    m_requiredLoginIndices = value;
}

bool Session::isFullyAuthenticated(const eCheckMode &eCheckMode)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    if (m_requiredLoginIndices.empty())
        return false;

    for (const auto & i : m_requiredLoginIndices)
    {
        auto currentAuthenticationForPassword = getIdxAuthenticationStatus_I(i.first);

        if ( eCheckMode == CHECK_DISALLOW_EXPIRED_PASSWORDS && currentAuthenticationForPassword!=REASON_AUTHENTICATED )
            return false;

        if ( eCheckMode == CHECK_ALLOW_EXPIRED_PASSWORDS && !IS_PASSWORD_AUTHENTICATED(currentAuthenticationForPassword) )
            return false;
    }
    return true;
}

std::string Session::getApplicationName()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    return m_applicationName;
}

void Session::setApplicationName(const std::string &value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    m_applicationName = value;
}

std::string Session::getAuthenticatedDomain()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    return m_authenticatedDomain;
}

void Session::setAuthenticatedDomain(const std::string &value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_authenticatedDomain = value;
}

void Session::setLastActivity(const time_t &value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_lastActivityTimestamp = value;
}

std::string Session::createNewSessionIDStr()
{
    return Mantids29::Helpers::Random::createRandomString(32);
}

void Session::setAuthUser(const std::string &value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_authenticatedUser = value;
}

time_t Session::getFirstActivity()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_firstActivityTimestamp;
}

std::string Session::getSessionId()
{ 
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_sessionID;
}

std::pair<std::string, std::string> Session::getUserDomainPair() const
{
    return std::make_pair(m_authenticatedUser,m_authenticatedDomain);
}

void Session::setSessionId(const std::string &value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_sessionID = value;
}

std::string Session::getAuthUser()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_authenticatedUser;
}

void Session::setIndexAuthenticationPolicy(uint32_t passIndex, const sAuthenticationPolicy &authPolicy)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_authenticationPolicies[passIndex] = authPolicy;
}

