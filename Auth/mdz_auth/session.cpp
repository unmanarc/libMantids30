#include "session.h"
#include <iomanip>
#include <sstream>
#include <mdz_hlp_functions/random.h>
#include <mdz_thr_mutex/lock_shared.h>

using namespace Mantids::Authentication;



Session::Session(const std::string &appName)
{
    this->isPersistentSession = false;
    this->appName = appName;
    regenSessionId();
}

Reason Session::getIdxAuthenticationStatus(uint32_t passIndex)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    return getIdxAuthenticationStatus_I(passIndex);

}

void Session::registerPersistentAuthentication(const std::string & sAccountName, const std::string &accountDomain, uint32_t passIndex, const Reason & reason)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    authMatrix[passIndex].lastAuthStatus = reason;
    authMatrix[passIndex].setCurrentTime();

    if ( IS_PASSWORD_AUTHENTICATED(reason) )
    {
        iUpdateLastActivity();
        firstActivity = lastActivity;
    }
    // Authenticated with the main password:
    if (!passIndex && IS_PASSWORD_AUTHENTICATED( reason ) )
    {
        authUser = sAccountName;
        authDomain = accountDomain;
    }
}

void Session::registerPersistentAuthentication(uint32_t passIndex, const Reason &reason)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    authMatrix[passIndex].lastAuthStatus = reason;
    authMatrix[passIndex].setCurrentTime();

    if ( IS_PASSWORD_AUTHENTICATED(reason) )
    {
        iUpdateLastActivity();
        firstActivity = lastActivity;
    }
}

void Session::updateLastActivity()
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    iUpdateLastActivity();
}

bool Session::isLastActivityExpired(const uint32_t &expSeconds)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    time_t curTime = time(nullptr);
    if (lastActivity>curTime) return true; // Computer time has changed?
    return (uint32_t)(curTime-lastActivity)>expSeconds;
}

time_t Session::getLastActivity()
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    return lastActivity;
}

void Session::regenSessionId()
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    sessionId = createNewSessionIDStr();
}

std::string Session::getUserID()
{
    return authUser;
}

sCurrentAuthentication Session::getCurrentAuthenticationStatus(const uint32_t &passIndex)
{
   // std::unique_lock<std::mutex> lock(mutexAuth); < private method.
    if (authMatrix.find(passIndex)!=authMatrix.end())
        return authMatrix[passIndex];
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
        if (authPolicies.find(passIndex)!=authPolicies.end())
        {
            // Then, if exist, check the max auth time.
            if (authPolicies[passIndex].validTime>0 && curAuthStatus.authTime+authPolicies[passIndex].validTime < time(nullptr))
                return REASON_EXPIRED;
        }
    }
    return curAuthStatus.lastAuthStatus;
}

void Session::iUpdateLastActivity()
{
    lastActivity = time(nullptr);

}

bool Session::getIsPersistentSession()
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    return isPersistentSession;
}

void Session::setIsPersistentSession(bool value)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    isPersistentSession = value;
}

std::map<uint32_t, std::string> Session::getRequiredLoginIdxs()
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    return requiredLoginIdxs;
}

std::pair<uint32_t, std::string> Session::getNextRequiredLoginIdxs()
{
    std::pair<uint32_t, std::string> r = std::make_pair(0xFFFFFFFF,"");
    std::unique_lock<std::mutex> lock(mutexAuth);

    for (const auto & i : requiredLoginIdxs)
    {
        // Get the first required password that is not authenticated (allow expired and valid pass):
        if ( !IS_PASSWORD_AUTHENTICATED(getIdxAuthenticationStatus_I(i.first)) )
            return i;
    }

    return r;
}

void Session::setRequiredLoginIdx(const std::map<uint32_t,std::string> &value)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    requiredLoginIdxs = value;
}

bool Session::getIsFullyLoggedIn(const eCheckMode &eCheckMode)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    if (requiredLoginIdxs.empty())
        return false;

    for (const auto & i : requiredLoginIdxs)
    {
        auto currentAuthenticationForPassword = getIdxAuthenticationStatus_I(i.first);

        if ( eCheckMode == CHECK_DISALLOW_EXPIRED_PASSWORDS && currentAuthenticationForPassword!=REASON_AUTHENTICATED )
            return false;

        if ( eCheckMode == CHECK_ALLOW_EXPIRED_PASSWORDS && !IS_PASSWORD_AUTHENTICATED(currentAuthenticationForPassword) )
            return false;
    }
    return true;
}

std::string Session::getAppName()
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    return appName;
}

void Session::setAppName(const std::string &value)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    appName = value;
}

std::string Session::getAuthDomain()
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    return authDomain;
}

void Session::setAuthDomain(const std::string &value)
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    authDomain = value;
}

void Session::setLastActivity(const time_t &value)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    lastActivity = value;
}

std::string Session::createNewSessionIDStr()
{
    return Mantids::Helpers::Random::createRandomString(32);
}

void Session::setAuthUser(const std::string &value)
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    authUser = value;
}

time_t Session::getFirstActivity()
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    return firstActivity;
}

std::string Session::getSessionId()
{ 
    std::unique_lock<std::mutex> lock(mutexAuth);
    return sessionId;
}

std::pair<std::string, std::string> Session::getUserDomainPair() const
{
    return std::make_pair(authUser,authDomain);
}

void Session::setSessionId(const std::string &value)
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    sessionId = value;
}


std::string Session::getAuthUser()
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    return authUser;
}

void Session::setIndexAuthenticationPolicy(uint32_t passIndex, const sAuthenticationPolicy &authPolicy)
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    authPolicies[passIndex] = authPolicy;
}

