#include "session.h"
#include <Mantids30/Helpers/random.h>
#include <Mantids30/Threads/lock_shared.h>

using namespace Mantids30::Auth;

Session::Session()
{
    //this->m_isPersistentSession = false;
    //this->m_applicationName = appName;
    this->m_firstActivityTimestamp = time(nullptr);
    regenerateSessionID();
}

std::string Session::getEffectiveUser()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    if (!m_impersonatedUser.empty())
        return m_impersonatedUser;
    else
        return m_authenticatedUser;
}
/*
Reason Session::getAuthenticationSlotStatus(uint32_t slotId)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    return getAuthenticationSlotStatus_I(slotId);
}*/
/*
void Session::registerPersistentAuthentication(const std::string & accountName, const std::string &accountDomain, uint32_t slotId, const Reason & reason)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    m_authenticationMatrix[slotId].lastAuthStatus = reason;
    m_authenticationMatrix[slotId].setCurrentTime();

    if ( IS_PASSWORD_AUTHENTICATED(reason) )
    {
        iUpdateLastActivity();
        m_firstActivityTimestamp = m_lastActivityTimestamp;
    }
    // Authenticated with the main password:
    if (!slotId && IS_PASSWORD_AUTHENTICATED( reason ) )
    {
        m_authenticatedUser = accountName;
        m_authenticatedDomain = accountDomain;
    }
}
*/
/*
void Session::registerPersistentAuthentication(uint32_t slotId, const Reason &reason)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    m_authenticationMatrix[slotId].lastAuthStatus = reason;
    m_authenticationMatrix[slotId].setCurrentTime();

    if ( IS_PASSWORD_AUTHENTICATED(reason) )
    {
        iUpdateLastActivity();
        m_firstActivityTimestamp = m_lastActivityTimestamp;
    }
}*/

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
/*
CurrentAuthenticationStatus Session::getCurrentAuthenticationStatus(const uint32_t &slotId)
{
   // std::unique_lock<std::mutex> lock(mutexAuth); < private method.
    if (m_authenticationMatrix.find(slotId)!=m_authenticationMatrix.end())
        return m_authenticationMatrix[slotId];
    CurrentAuthenticationStatus x;
    return x;
}*/
/*
Reason Session::getAuthenticationSlotStatus_I(uint32_t slotId)
{
    CurrentAuthenticationStatus curAuthStatus;

    curAuthStatus = getCurrentAuthenticationStatus(slotId);
    if ( IS_PASSWORD_AUTHENTICATED( curAuthStatus.lastAuthStatus ) )
    {
        // If authenticated: check if exist a policy for that slotId
        if (m_authenticationPolicies.find(slotId)!=m_authenticationPolicies.end())
        {
            // Then, if exist, check the max auth time.
            if (m_authenticationPolicies[slotId].validTime>0 && curAuthStatus.authTime+m_authenticationPolicies[slotId].validTime < time(nullptr))
                return REASON_EXPIRED;
        }
    }
    return curAuthStatus.lastAuthStatus;
}
*/
void Session::iUpdateLastActivity()
{
    m_lastActivityTimestamp = time(nullptr);
}
/*
std::string Session::impersonatedUser()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_impersonatedUser;
}*/

void Session::setImpersonatedUser(const std::string &newImpersonatedUser)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_impersonatedUser = newImpersonatedUser;
}

std::map<std::string, json> Session::getClaims()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_claims;
}

json Session::getClaim(const std::string &claimName)
{
    json r;
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    if ( m_claims.find(claimName) == m_claims.end() )
        return r;

    return m_claims[claimName];
}

void Session::setClaims(const std::map<std::string, json> &newClaims)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_claims = newClaims;
}

void Session::addClaims(const std::map<std::string, json> &newClaims)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    for (const auto &i : newClaims)
    {
        m_claims[i.first] = i.second;
    }
}

bool Session::validateAppPermissionInClaim(const std::string &permissionName)
{
    // TODO: if there is a connection validation, we can call back the validation server.
    std::set<std::string> permissions = Helpers::jsonToStringSet( getClaim("permissions") );
    return permissions.find(permissionName)!=permissions.end();
}

bool Session::isAdmin()
{
    // TODO: validate to the server...
    return JSON_ASBOOL_D(getClaim("admin"),false);
}

std::set<std::string> Session::getActivities()
{
    // TODO: validate to the server...
    return Helpers::jsonToStringSet( getClaim("activities") );
}

/*
bool Session::isPersistentSession()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_isPersistentSession;
}

void Session::setIsPersistentSession(bool value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_isPersistentSession = value;
}*/
/*
std::map<uint32_t, std::string> Session::getRequiredAuthenticationSlotsForLogin()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    return m_requiredAuthenticationSlotsForLogin;
}
*//*
std::pair<uint32_t, std::string> Session::getNextRequiredAuthenticationSlotIdForLogin()
{
    std::pair<uint32_t, std::string> r = std::make_pair(0xFFFFFFFF,"");
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    for (const auto & i : m_requiredAuthenticationSlotsForLogin)
    {
        // Get the first required password that is not authenticated (allow expired and valid pass):
        if ( !IS_PASSWORD_AUTHENTICATED(getAuthenticationSlotStatus_I(i.first)) )
            return i;
    }

    return r;
}
*/
/*
void Session::setRequiredAuthenticationSlotsForLogin(const std::map<uint32_t, std::string> &authenticationSlots)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    m_requiredAuthenticationSlotsForLogin = authenticationSlots;
}
*/
/*
bool Session::isFullyAuthenticated(const eCheckMode &eCheckMode)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    if (m_requiredAuthenticationSlotsForLogin.empty())
        return false;

    for (const auto & i : m_requiredAuthenticationSlotsForLogin)
    {
        auto currentAuthenticationForPassword = getAuthenticationSlotStatus_I(i.first);

        if ( eCheckMode == CHECK_DISALLOW_EXPIRED_PASSWORDS && currentAuthenticationForPassword!=REASON_AUTHENTICATED )
            return false;

        if ( eCheckMode == CHECK_ALLOW_EXPIRED_PASSWORDS && !IS_PASSWORD_AUTHENTICATED(currentAuthenticationForPassword) )
            return false;
    }
    return true;
}
*/
/*
std::string Session::getApplicationName()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    return m_applicationName;
}

void Session::setApplicationName(const std::string &value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    m_applicationName = value;
}*/
/*
std::string Session::getAuthenticatedDomain()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);

    return m_authenticatedDomain;
}

void Session::setAuthenticatedDomain(const std::string &value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_authenticatedDomain = value;
}*/

void Session::setLastActivity(const time_t &value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_lastActivityTimestamp = value;
}

std::string Session::createNewSessionIDStr()
{
    return Mantids30::Helpers::Random::createRandomString(32);
}

void Session::setAuthenticatedUser(const std::string &value)
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
/*
std::pair<std::string, std::string> Session::getUserDomainPair() const
{
    return std::make_pair(m_authenticatedUser,m_authenticatedDomain);
}
*/
void Session::setSessionId(const std::string &value)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_sessionID = value;
}
/*
std::string Session::getAuthUser()
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    return m_authenticatedUser;
}*/
/*
void Session::setSlotIdAuthenticationPolicy(uint32_t slotId, const SlotAuthenticationPolicy &authPolicy)
{
    std::unique_lock<std::mutex> lock(m_authenticationMutex);
    m_authenticationPolicies[slotId] = authPolicy;
}*/

