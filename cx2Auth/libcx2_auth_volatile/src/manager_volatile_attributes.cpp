#include "manager_volatile.h"
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Authentication;

bool Manager_Volatile::attribAdd(const std::string &attribName, const std::string &attribDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    if (attribs.find(attribName) != attribs.end()) return false;
    attribs[attribName].name = attribName;
    attribs[attribName].description = attribDescription;
    return true;
}

bool Manager_Volatile::attribRemove(const std::string &attribName)
{
    Threads::Sync::Lock_RW lock(mutex);

    for ( auto & acct : attribAccounts(attribName,false) )
    {
        accounts[acct].accountAttribs.erase(attribName);
    }

    for ( auto & grp : attribGroups(attribName,false) )
    {
        groups[grp].groupAttribs.erase(attribName);
    }

    if (attribs.find(attribName) == attribs.end()) return false;
    attribs.erase(attribName);

    return true;
}

bool Manager_Volatile::attribGroupAdd(const std::string &attribName, const std::string &groupName)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (groups.find(groupName) == groups.end()) return false;
    if (attribs.find(attribName) == attribs.end()) return false;

    if (groups[groupName].groupAttribs.find(attribName) != groups[groupName].groupAttribs.end()) return false;

    groups[groupName].groupAttribs.insert(attribName);

    return true;
}

bool Manager_Volatile::attribGroupRemove(const std::string &attribName, const std::string &groupName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();

    if (    groups.find(groupName) != groups.end() &&
            attribs.find(attribName) != attribs.end() &&
            groups[groupName].groupAttribs.find(attribName) != groups[groupName].groupAttribs.end()
            )
    {
        groups[groupName].groupAttribs.erase(attribName);
        ret = true;
    }
    if (lock) mutex.unlock();
    return ret;
}

bool Manager_Volatile::attribAccountAdd(const std::string &attribName, const std::string &accountName)
{
    bool ret = false;

    Threads::Sync::Lock_RW lock(mutex);

    if (    accounts.find(accountName) != accounts.end() &&
            attribs.find(attribName) != attribs.end() &&
            accounts[accountName].accountAttribs.find(attribName) == accounts[accountName].accountAttribs.end()
            )
    {
        accounts[accountName].accountAttribs.insert(attribName);
        ret = true;
    }

    return ret;
}

bool Manager_Volatile::attribAccountRemove(const std::string &attribName, const std::string &accountName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();

    if (    accounts.find(accountName) != accounts.end() &&
            attribs.find(attribName) != attribs.end() &&
            accounts[accountName].accountAttribs.find(attribName) != accounts[accountName].accountAttribs.end()
            )
    {
        accounts[accountName].accountAttribs.erase(attribName);
        ret = true;
    }
    if (lock) mutex.unlock();
    return ret;
}

bool Manager_Volatile::attribChangeDescription(const std::string &attribName, const std::string &attribDescription)
{

    Threads::Sync::Lock_RW lock(mutex);
    if (attribs.find(attribName) == attribs.end()) return false;
    attribs[attribName].description = attribDescription;
    return true;
}

std::string Manager_Volatile::attribDescription(const std::string &attribName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (attribs.find(attribName) == attribs.end()) return "";
    return attribs[attribName].description;
}

std::set<std::string> Manager_Volatile::attribsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);
    for (const auto &i : attribs) ret.insert(i.first);
    return ret;
}

std::set<std::string> Manager_Volatile::attribGroups(const std::string &attribName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    for (const auto&  i : groups)
    {
        if (i.second.groupAttribs.find(attribName) != i.second.groupAttribs.end())
        {
            ret.insert(i.first);
        }
    }
    if (lock) mutex.unlock_shared();

    return ret;
}

std::set<std::string> Manager_Volatile::attribAccounts(const std::string &attribName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    for (const auto&  i : accounts)
    {
        if (i.second.accountAttribs.find(attribName) != i.second.accountAttribs.end())
        {
            ret.insert(i.first);
        }
    }
    if (lock) mutex.unlock_shared();

    return ret;
}

bool Manager_Volatile::accountValidateDirectAttribute(const std::string &accountName, const std::string &attribName)
{
    bool ret=false;
    Threads::Sync::Lock_RD lock(mutex);
    if (accounts.find(accountName) != accounts.end() && accounts[accountName].accountAttribs.find(attribName) != accounts[accountName].accountAttribs.end())
    {
        ret = true;
    }
    return ret;
}
