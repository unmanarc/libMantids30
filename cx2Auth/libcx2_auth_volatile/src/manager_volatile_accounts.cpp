#include "manager_volatile.h"

#include <limits>
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Authentication;

bool Manager_Volatile::accountAdd(const std::string &sUserName, const Secret &secretData, const std::string &email, const std::string &accountDescription, const std::string &extraData, time_t expirationDate, bool enabled, bool confirmed, bool superuser)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (accounts.find(sUserName) != accounts.end()) return false;

    accounts[sUserName].name = sUserName;
    accounts[sUserName].email = email;
    accounts[sUserName].description = accountDescription;
    accounts[sUserName].extraData = extraData;
    accounts[sUserName].superuser = superuser;

    accounts[sUserName].enabled = enabled;
    accounts[sUserName].expirationDate = expirationDate;
    accounts[sUserName].confirmed = confirmed;
    accounts[sUserName].confirmedToken =  genRandomConfirmationToken();

    accounts[sUserName].passwordByIDX[0] = secretData;

    return true;
}

std::string Manager_Volatile::accountConfirmationToken(const std::string &sUserName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);

    if (accounts.find(sUserName) == accounts.end()) return "";
    return accounts[sUserName].confirmedToken;
}

bool Manager_Volatile::accountRemove(const std::string &sUserName)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return false;
    accounts.erase(sUserName);
    return true;
}

bool Manager_Volatile::accountExist(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return false;
    return true;
}

bool Manager_Volatile::accountDisable(const std::string &sUserName, bool disabled)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (accounts.find(sUserName) == accounts.end()) return false;
    accounts[sUserName].enabled = !disabled;
    return true;
}

bool Manager_Volatile::accountConfirm(const std::string &sUserName, const std::string &confirmationToken)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return false;
    if (accounts[sUserName].confirmedToken == confirmationToken)
    {
        accounts[sUserName].confirmed = true;
        return true;
    }
    return false;
}

bool Manager_Volatile::accountChangeSecret(const std::string &sUserName, const Secret &passwordData, uint32_t passIndex)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return false;
    accounts[sUserName].passwordByIDX[passIndex] = passwordData;
    return true;
}

bool Manager_Volatile::accountChangeDescription(const std::string &sUserName, const std::string &description)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return false;
    accounts[sUserName].description = description;
    return true;
}

bool Manager_Volatile::accountChangeEmail(const std::string &sUserName, const std::string &email)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return false;
    accounts[sUserName].email = email;
    return true;
}

bool Manager_Volatile::accountChangeExtraData(const std::string &sUserName, const std::string &extraData)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return false;
    accounts[sUserName].extraData = extraData;
    return true;
}

bool Manager_Volatile::accountChangeExpiration(const std::string &sUserName, time_t expiration)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return false;
    accounts[sUserName].expirationDate = expiration;
    return true;
}

bool Manager_Volatile::isAccountDisabled(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return true;
    return !(accounts[sUserName].enabled);
}

bool Manager_Volatile::isAccountConfirmed(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return false;
    return accounts[sUserName].confirmed;
}

bool Manager_Volatile::isAccountSuperUser(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return false;
    return accounts[sUserName].superuser;
}

std::string Manager_Volatile::accountDescription(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return "";
    return accounts[sUserName].description;
}

std::string Manager_Volatile::accountEmail(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return "";
    return accounts[sUserName].email;
}

std::string Manager_Volatile::accountExtraData(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return "";
    return accounts[sUserName].extraData;
}

time_t Manager_Volatile::accountExpirationDate(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(sUserName) == accounts.end()) return 0;
    return accounts[sUserName].expirationDate;
}

std::set<std::string> Manager_Volatile::accountsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);
    for (const auto &i : accounts) ret.insert(i.first);
    return ret;
}

std::set<std::string> Manager_Volatile::accountGroups(const std::string &sUserName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();
    if (accounts.find(sUserName)!=accounts.end())
    {
        for (const auto &i : accounts[sUserName].accountGroups) ret.insert(i);
    }
    if (lock) mutex.unlock_shared();
    return ret;
}

std::set<std::string> Manager_Volatile::accountDirectAttribs(const std::string &sUserName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();
    if (accounts.find(sUserName)!=accounts.end())
    {
        for (const auto &i : accounts[sUserName].accountAttribs) ret.insert(i);
    }
    if (lock) mutex.unlock_shared();
    return ret;
}

Secret Manager_Volatile::retrieveSecret(const std::string &sUserName, uint32_t passIndex, bool *found)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(sUserName) != accounts.end() && accounts[sUserName].passwordByIDX.find(passIndex) != accounts[sUserName].passwordByIDX.end() )
    {
        *found = true;
        return accounts[sUserName].passwordByIDX[passIndex];
    }
    else
    {
        Secret r;
        *found = false;
        return r;
    }
}
