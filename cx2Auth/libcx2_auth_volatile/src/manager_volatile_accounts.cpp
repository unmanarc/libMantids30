#include "manager_volatile.h"

#include <limits>
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Authentication;

bool Manager_Volatile::accountAdd(const std::string &accountName, const Secret &passData, const std::string &email, const std::string &accountDescription, const std::string &extraData, time_t expirationDate, bool enabled, bool confirmed, bool superuser)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (accounts.find(accountName) != accounts.end()) return false;

    accounts[accountName].name = accountName;
    accounts[accountName].email = email;
    accounts[accountName].description = accountDescription;
    accounts[accountName].extraData = extraData;
    accounts[accountName].superuser = superuser;

    accounts[accountName].enabled = enabled;
    accounts[accountName].expirationDate = expirationDate;
    accounts[accountName].confirmed = confirmed;
    accounts[accountName].confirmedToken =  genRandomConfirmationToken();

    accounts[accountName].passwordByIDX[0] = passData;

    return true;
}

std::string Manager_Volatile::accountConfirmationToken(const std::string &accountName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);

    if (accounts.find(accountName) == accounts.end()) return "";
    return accounts[accountName].confirmedToken;
}

bool Manager_Volatile::accountRemove(const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return false;
    accounts.erase(accountName);
    return true;
}

bool Manager_Volatile::accountExist(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return false;
    return true;
}

bool Manager_Volatile::accountDisable(const std::string &accountName, bool disabled)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (accounts.find(accountName) == accounts.end()) return false;
    accounts[accountName].enabled = !disabled;
    return true;
}

bool Manager_Volatile::accountConfirm(const std::string &accountName, const std::string &confirmationToken)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return false;
    if (accounts[accountName].confirmedToken == confirmationToken)
    {
        accounts[accountName].confirmed = true;
        return true;
    }
    return false;
}

bool Manager_Volatile::accountChangeSecret(const std::string &accountName, const Secret &passwordData, uint32_t passIndex)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return false;
    accounts[accountName].passwordByIDX[passIndex] = passwordData;
    return true;
}

bool Manager_Volatile::accountChangeDescription(const std::string &accountName, const std::string &description)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return false;
    accounts[accountName].description = description;
    return true;
}

bool Manager_Volatile::accountChangeEmail(const std::string &accountName, const std::string &email)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return false;
    accounts[accountName].email = email;
    return true;
}

bool Manager_Volatile::accountChangeExtraData(const std::string &accountName, const std::string &extraData)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return false;
    accounts[accountName].extraData = extraData;
    return true;
}

bool Manager_Volatile::accountChangeExpiration(const std::string &accountName, time_t expiration)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return false;
    accounts[accountName].expirationDate = expiration;
    return true;
}

bool Manager_Volatile::isAccountDisabled(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return true;
    return !(accounts[accountName].enabled);
}

bool Manager_Volatile::isAccountConfirmed(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return false;
    return accounts[accountName].confirmed;
}

bool Manager_Volatile::isAccountSuperUser(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return false;
    return accounts[accountName].superuser;
}

std::string Manager_Volatile::accountDescription(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return "";
    return accounts[accountName].description;
}

std::string Manager_Volatile::accountEmail(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return "";
    return accounts[accountName].email;
}

std::string Manager_Volatile::accountExtraData(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return "";
    return accounts[accountName].extraData;
}

time_t Manager_Volatile::accountExpirationDate(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(accountName) == accounts.end()) return 0;
    return accounts[accountName].expirationDate;
}

std::set<std::string> Manager_Volatile::accountsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);
    for (const auto &i : accounts) ret.insert(i.first);
    return ret;
}

std::set<std::string> Manager_Volatile::accountGroups(const std::string &accountName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();
    if (accounts.find(accountName)!=accounts.end())
    {
        for (const auto &i : accounts[accountName].accountGroups) ret.insert(i);
    }
    if (lock) mutex.unlock_shared();
    return ret;
}

std::set<std::string> Manager_Volatile::accountDirectAttribs(const std::string &accountName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();
    if (accounts.find(accountName)!=accounts.end())
    {
        for (const auto &i : accounts[accountName].accountAttribs) ret.insert(i);
    }
    if (lock) mutex.unlock_shared();
    return ret;
}

Secret Manager_Volatile::retrieveSecret(const std::string &accountName, uint32_t passIndex, bool *found)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(accountName) != accounts.end() && accounts[accountName].passwordByIDX.find(passIndex) != accounts[accountName].passwordByIDX.end() )
    {
        *found = true;
        return accounts[accountName].passwordByIDX[passIndex];
    }
    else
    {
        Secret r;
        *found = false;
        return r;
    }
}
