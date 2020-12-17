#include "session.h"
#include <random>
#include <iomanip>
#include <sstream>
#include <cx2_hlp_functions/random.h>

using namespace CX2::Authentication;

Session::Session()
{
    regenSessionId();
}

Reason Session::isAuthenticated(uint32_t passIndex)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    sCurrentAuthentication curAuth = getCurrentAuthentication(passIndex);
    if (curAuth.lastReason == REASON_AUTHENTICATED)
    {
        // If authenticated: check policy
        if (authPolicies.find(passIndex)!=authPolicies.end())
        {
           if (authPolicies[passIndex].validTime>0 && curAuth.authTime+authPolicies[passIndex].validTime < time(nullptr)) return REASON_EXPIRED;
        }
    }
    return curAuth.lastReason;
}

void Session::registerPersistentAuthentication(const std::string & accountName, const std::string &accountDomain, uint32_t passIndex, const Reason & reason)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    authMatrix[passIndex].lastReason = reason;
    authMatrix[passIndex].setCurrentTime();

    if (reason == REASON_AUTHENTICATED)
    {
        updateLastActivity();
        firstActivity = lastActivity;
    }
    // Authenticated with the main password:
    if (!passIndex && reason==REASON_AUTHENTICATED)
    {
        authUser = accountName;
        authDomain = accountDomain;
    }
}

void Session::registerPersistentAuthentication(uint32_t passIndex, const Reason &reason)
{
    std::unique_lock<std::mutex> lock(mutexAuth);

    authMatrix[passIndex].lastReason = reason;
    authMatrix[passIndex].setCurrentTime();

    if (reason == REASON_AUTHENTICATED)
    {
        updateLastActivity();
        firstActivity = lastActivity;
    }
}

void Session::updateLastActivity()
{
    lastActivity = time(nullptr);
}

bool Session::isLastActivityExpired(const uint32_t &expSeconds) const
{
    return (time(nullptr)-lastActivity)>expSeconds;
}

time_t Session::getLastActivity() const
{
    return lastActivity;
}

void Session::regenSessionId()
{
    sessionId = createNewSessionIDStr();
}

std::string Session::getUserID()
{
    return authUser;
}

sCurrentAuthentication Session::getCurrentAuthentication(const uint32_t &passIndex)
{
    if (authMatrix.find(passIndex)!=authMatrix.end())
        return authMatrix[passIndex];
    sCurrentAuthentication x;
    return x;
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
    lastActivity = value;
}

std::string Session::createNewSessionIDStr()
{
    return CX2::Helpers::Random::createRandomString(32);
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

