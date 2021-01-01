#include "manager_db.h"
#include <cx2_thr_mutex/lock_shared.h>

#include <cx2_mem_vars/a_string.h>

using namespace CX2::Authentication;
using namespace CX2::Memory;
using namespace CX2::Database;

bool Manager_DB::attribAdd(const std::string &attribName, const std::string &attribDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("INSERT INTO vauth_v2_attribs (name,description) VALUES(:name,:description);",
                               {
                                   {":name",Abstract::STRING(attribName)},
                                   {":description",Abstract::STRING(attribDescription)}
                               });
}

bool Manager_DB::attribRemove(const std::string &attribName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("DELETE FROM vauth_v2_attribs WHERE name=:name;",
                               {
                                   {":name",Abstract::STRING(attribName)}
                               });
}

bool Manager_DB::attribExist(const std::string &attribName)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(mutex);

    QueryInstance i = sqlConnector->query("SELECT description FROM vauth_v2_attribs WHERE name=:name LIMIT 1;",
                                          {{":name",Memory::Abstract::STRING(attribName)}},
                                          { });
    if (i.ok && i.query->step())
    {
        ret = true;
    }
    return ret;
}

bool Manager_DB::attribGroupAdd(const std::string &attribName, const std::string &groupName)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("INSERT INTO vauth_v2_attribs_groups (`attrib_name`,`group_name`) VALUES(:name,:group);",
                               {
                                   {":name",Abstract::STRING(attribName)},
                                   {":group",Abstract::STRING(groupName)}
                               });
}

bool Manager_DB::attribGroupRemove(const std::string &attribName, const std::string &groupName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();
    ret = sqlConnector->query("DELETE FROM vauth_v2_attribs_groups WHERE attrib_name=:name AND group_name=:group;",
                              {
                                  {":name",Abstract::STRING(attribName)},
                                  {":group",Abstract::STRING(groupName)}
                              });
    if (lock) mutex.unlock();
    return ret;
}

bool Manager_DB::attribAccountAdd(const std::string &attribName, const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("INSERT INTO vauth_v2_attribs_accounts (`attrib_name`,`account_name`) VALUES(:name,:account);",
                               {
                                   {":name",Abstract::STRING(attribName)},
                                   {":account",Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::attribAccountRemove(const std::string &attribName, const std::string &accountName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();
    ret = sqlConnector->query("DELETE FROM vauth_v2_attribs_accounts WHERE attrib_name=:name AND account_name=:account;",
                              {
                                  {":name",Abstract::STRING(attribName)},
                                  {":account",Abstract::STRING(accountName)}
                              });
    if (lock) mutex.unlock();
    return ret;
}

bool Manager_DB::attribChangeDescription(const std::string &attribName, const std::string &attribDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v2_attribs SET description=:description WHERE name=:name;",
                               {
                                   {":name",Abstract::STRING(attribName)},
                                   {":description",Abstract::STRING(attribDescription)}
                               });
}

std::string Manager_DB::attribDescription(const std::string &attribName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING description;
    QueryInstance i = sqlConnector->query("SELECT description FROM vauth_v2_attribs WHERE name=:name LIMIT 1;",
                                          {{":name",Memory::Abstract::STRING(attribName)}},
                                          { &description });
    if (i.ok && i.query->step())
    {
        return description.getValue();
    }
    return "";
}

std::set<std::string> Manager_DB::attribsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING name;
    QueryInstance i = sqlConnector->query("SELECT name FROM vauth_v2_attribs;",
                                          {},
                                          { &name });
    while (i.ok && i.query->step())
    {
        ret.insert(name.getValue());
    }
    return ret;
}

std::set<std::string> Manager_DB::attribGroups(const std::string &attribName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING gruopName;
    QueryInstance i = sqlConnector->query("SELECT group_name FROM vauth_v2_attribs_groups WHERE `attrib_name`=:attrib;",
                                          { {":attrib",Memory::Abstract::STRING(attribName)} },
                                          { &gruopName });
    while (i.ok && i.query->step())
    {
        ret.insert(gruopName.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

std::set<std::string> Manager_DB::attribAccounts(const std::string &attribName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();


    Abstract::STRING accountName;
    QueryInstance i = sqlConnector->query("SELECT account_name FROM vauth_v2_attribs_accounts WHERE `attrib_name`=:attrib;",
                                          { {":attrib",Memory::Abstract::STRING(attribName)} },
                                          { &accountName });
    while (i.ok && i.query->step())
    {
        ret.insert(accountName.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

bool Manager_DB::accountValidateDirectAttribute(const std::string &accountName, const std::string &attribName)
{
    Threads::Sync::Lock_RD lock(mutex);

    QueryInstance i = sqlConnector->query("SELECT account_name FROM vauth_v2_attribs_accounts WHERE `attrib_name`=:attrib AND `account_name`=:account;",
                                          { {":attrib",Memory::Abstract::STRING(attribName)},
                                            {":account",Memory::Abstract::STRING(accountName)}
                                          },
                                          { });
    return (i.ok && i.query->step());
}
