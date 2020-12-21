#include "manager_db.h"
#include <cx2_thr_mutex/lock_shared.h>
#include <cx2_mem_vars/a_string.h>

using namespace CX2::Authentication;
using namespace CX2::Memory;
using namespace CX2::Database;

bool Manager_DB::groupAdd(const std::string &groupName, const std::string &groupDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("INSERT INTO vauth_v2_groups (name,description) VALUES(:name,:description);",
                               {
                                   {":name",Abstract::STRING(groupName)},
                                   {":description",Abstract::STRING(groupDescription)}
                               });
}

bool Manager_DB::groupRemove(const std::string &groupName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("DELETE FROM vauth_v2_groups WHERE name=:name;",
                               {
                                   {":name",Abstract::STRING(groupName)}
                               });
}

bool Manager_DB::groupExist(const std::string &groupName)
{
    Threads::Sync::Lock_RD lock(mutex);
    QueryInstance i = sqlConnector->query("SELECT name FROM vauth_v2_groups WHERE `name`=:name;",
                                          {{":name",Memory::Abstract::STRING(groupName)}},
                                          {});
    return i.ok && i.query->step();
}

bool Manager_DB::groupAccountAdd(const std::string &groupName, const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("INSERT INTO vauth_v2_groups_accounts (`group_name`,`account_name`) VALUES(:group,:account);",
                               {
                                   {":group",Abstract::STRING(groupName)},
                                   {":account",Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::groupAccountRemove(const std::string &groupName, const std::string &accountName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();
    ret = sqlConnector->query("DELETE FROM vauth_v2_groups_accounts WHERE group_name=:group AND account_name=:account;",
                              {
                                  {":group",Abstract::STRING(groupName)},
                                  {":account",Abstract::STRING(accountName)}
                              });

    if (lock) mutex.unlock();
    return ret;
}

bool Manager_DB::groupChangeDescription(const std::string &groupName, const std::string &groupDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v2_groups SET description=:name WHERE name=:description;",
                               {
                                   {":name",Abstract::STRING(groupName)},
                                   {":description",Abstract::STRING(groupDescription)}
                               });
}

bool Manager_DB::groupValidateAttribute(const std::string &groupName, const std::string &attribName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock_shared();

    QueryInstance i = sqlConnector->query("SELECT group_name FROM vauth_v2_attribs_groups WHERE `attrib_name`=:attrib AND `group_name`=:group;",
                                          {
                                              {":attrib",Memory::Abstract::STRING(attribName)},
                                              {":group",Memory::Abstract::STRING(groupName)}
                                          },
                                          {});
    ret = i.ok && i.query->step();

    if (lock) mutex.unlock_shared();
    return ret;
}

std::string Manager_DB::groupDescription(const std::string &groupName)
{
    Threads::Sync::Lock_RD lock(mutex);
    Abstract::STRING description;
    QueryInstance i = sqlConnector->query("SELECT description FROM vauth_v2_groups WHERE name=:name LIMIT 1;",
                                          {{":name",Memory::Abstract::STRING(groupName)}},
                                          { &description });
    if (i.ok && i.query->step())
    {
        return description.getValue();
    }
    return "";
}

std::set<std::string> Manager_DB::groupsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING name;
    QueryInstance i = sqlConnector->query("SELECT name FROM vauth_v2_groups;",
                                          {},
                                          { &name });
    while (i.ok && i.query->step())
    {
        ret.insert(name.getValue());
    }
    return ret;
}

std::set<std::string> Manager_DB::groupAttribs(const std::string &groupName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING attribName;
    QueryInstance i = sqlConnector->query("SELECT attrib_name FROM vauth_v2_attribs_groups WHERE group_name=:group;",
                                          { {":group",Memory::Abstract::STRING(groupName)} },
                                          { &attribName });
    while (i.ok && i.query->step())
    {
        ret.insert(attribName.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

std::set<std::string> Manager_DB::groupAccounts(const std::string &groupName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING accountName;
    QueryInstance i = sqlConnector->query("SELECT account_name FROM vauth_v2_groups_accounts WHERE group_name=:group;",
                                          { {":group",Memory::Abstract::STRING(groupName)} },
                                          { &accountName });
    while (i.ok && i.query->step())
    {
        ret.insert(accountName.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

