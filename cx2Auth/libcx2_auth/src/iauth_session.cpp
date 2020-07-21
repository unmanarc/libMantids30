#include "iauth_session.h"
#include <random>
#include <iomanip>
#include <sstream>
#include <cx2_hlp_functions/random.h>

using namespace CX2::Authorization;
using namespace CX2::Authorization::Session;
using namespace CX2::Authorization::DataStructs;

IAuth_Session::IAuth_Session()
{
    regenSessionId();
}

AuthReason IAuth_Session::isAuthenticated(uint32_t passIndex)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    sCurrentAuthentication curAuth = getCurrentAuthentication(passIndex);
    if (curAuth.lastAuthReason == AUTH_REASON_AUTHENTICATED)
    {
        // If authenticated: check policy
        if (authPolicies.find(passIndex)!=authPolicies.end())
        {
           if (authPolicies[passIndex].validTime>0 && curAuth.authTime+authPolicies[passIndex].validTime < time(nullptr)) return AUTH_REASON_EXPIRED;
        }
    }
    return curAuth.lastAuthReason;
}

void IAuth_Session::registerPersistentAuthentication(const std::string & accountName, const std::string &accountDomain, uint32_t passIndex, const AuthReason & reason)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    authMatrix[passIndex].lastAuthReason = reason;
    authMatrix[passIndex].setCurrentTime();

    if (reason == AUTH_REASON_AUTHENTICATED)
    {
        updateLastActivity();
        firstActivity = lastActivity;
    }
    // Authenticated with the main password:
    if (!passIndex && reason==AUTH_REASON_AUTHENTICATED)
    {
        authUser = accountName;
        authDomain = accountDomain;
    }
}

void IAuth_Session::updateLastActivity()
{
    lastActivity = time(nullptr);
}

bool IAuth_Session::isLastActivityExpired(const uint32_t &expSeconds) const
{
    return (time(nullptr)-lastActivity)>expSeconds;
}

time_t IAuth_Session::getLastActivity() const
{
    return lastActivity;
}

void IAuth_Session::regenSessionId()
{
    sessionId = createNewSessionIDStr();
}

std::string IAuth_Session::getUserID()
{
    return authUser;
}

sCurrentAuthentication IAuth_Session::getCurrentAuthentication(const uint32_t &passIndex)
{
    if (authMatrix.find(passIndex)!=authMatrix.end())
        return authMatrix[passIndex];
    sCurrentAuthentication x;
    return x;
}

std::string IAuth_Session::getAuthDomain()
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    return authDomain;
}

void IAuth_Session::setAuthDomain(const std::string &value)
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    authDomain = value;
}

void IAuth_Session::setLastActivity(const time_t &value)
{
    lastActivity = value;
}

std::string IAuth_Session::createNewSessionIDStr()
{
    return CX2::Helpers::Random::createRandomString(32);
}

void IAuth_Session::setAuthUser(const std::string &value)
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    authUser = value;
}

time_t IAuth_Session::getFirstActivity()
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    return firstActivity;
}

std::string IAuth_Session::getSessionId()
{ 
    std::unique_lock<std::mutex> lock(mutexAuth);
    return sessionId;
}

std::pair<std::string, std::string> IAuth_Session::getUserDomainPair() const
{
    return std::make_pair(authUser,authDomain);
}

void IAuth_Session::setSessionId(const std::string &value)
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    sessionId = value;
}


std::string IAuth_Session::getAuthUser()
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    return authUser;
}

void IAuth_Session::setIndexAuthenticationPolicy(uint32_t passIndex, const sAuthenticationPolicy &authPolicy)
{
    std::unique_lock<std::mutex> lock(mutexAuth);
    authPolicies[passIndex] = authPolicy;
}

