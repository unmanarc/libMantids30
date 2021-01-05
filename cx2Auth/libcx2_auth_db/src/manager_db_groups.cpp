#include "manager_db.h"
#include <cx2_thr_mutex/lock_shared.h>
#include <cx2_mem_vars/a_string.h>

using namespace CX2::Authentication;
using namespace CX2::Memory;
using namespace CX2::Database;

bool Manager_DB::groupAdd(const std::string &groupName, const std::string &groupDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("INSERT INTO vauth_v3_groups (`groupname`,`description`) VALUES(:groupname,:description);",
                               {
                                   {":groupname",new Abstract::STRING(groupName)},
                                   {":description",new Abstract::STRING(groupDescription)}
                               });
}

bool Manager_DB::groupRemove(const std::string &groupName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("DELETE FROM vauth_v3_groups WHERE `groupname`=:groupname;",
                               {
                                   {":groupname",new Abstract::STRING(groupName)}
                               });
}

bool Manager_DB::groupExist(const std::string &groupName)
{
    Threads::Sync::Lock_RD lock(mutex);
    QueryInstance i = sqlConnector->query("SELECT `groupname` FROM vauth_v3_groups WHERE `groupname`=:groupname;",
                                          {{":groupname",new Memory::Abstract::STRING(groupName)}},
                                          {});
    return i.ok && i.query->step();
}

bool Manager_DB::groupAccountAdd(const std::string &sGroupName, const std::string &sUserName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("INSERT INTO vauth_v3_groupsaccounts (`f_groupname`,`f_username`) VALUES(:groupname,:username);",
                               {
                                   {":groupname",new Abstract::STRING(sGroupName)},
                                   {":username",new Abstract::STRING(sUserName)}
                               });
}

bool Manager_DB::groupAccountRemove(const std::string &sGroupName, const std::string &sUserName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();
    ret = sqlConnector->query("DELETE FROM vauth_v3_groupsaccounts WHERE `f_groupname`=:groupname AND `f_username`=:username;",
                              {
                                  {":groupname",new Abstract::STRING(sGroupName)},
                                  {":username",new Abstract::STRING(sUserName)}
                              });

    if (lock) mutex.unlock();
    return ret;
}

bool Manager_DB::groupChangeDescription(const std::string &groupName, const std::string &groupDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v3_groups SET `description`=:description WHERE `groupname`=:groupname;",
                               {
                                   {":groupname",new Abstract::STRING(groupName)},
                                   {":description",new Abstract::STRING(groupDescription)}
                               });
}

bool Manager_DB::groupValidateAttribute(const std::string &sGroupName, const std::string &sAttribName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock_shared();

    QueryInstance i = sqlConnector->query("SELECT `f_groupname` FROM vauth_v3_attribsgroups WHERE `f_attribname`=:attribname AND `f_groupname`=:groupname;",
                                          {
                                              {":attribname",new Memory::Abstract::STRING(sAttribName)},
                                              {":groupname",new Memory::Abstract::STRING(sGroupName)}
                                          },
                                          {});
    ret = i.ok && i.query->step();

    if (lock) mutex.unlock_shared();
    return ret;
}

std::string Manager_DB::groupDescription(const std::string &sGroupName)
{
    Threads::Sync::Lock_RD lock(mutex);
    Abstract::STRING description;
    QueryInstance i = sqlConnector->query("SELECT `description` FROM vauth_v3_groups WHERE `groupname`=:groupname LIMIT 1;",
                                          {{":groupname",new Memory::Abstract::STRING(sGroupName)}},
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

    Abstract::STRING sGroupName;
    QueryInstance i = sqlConnector->query("SELECT `groupname` FROM vauth_v3_groups;",
                                          {},
                                          { &sGroupName });
    while (i.ok && i.query->step())
    {
        ret.insert(sGroupName.getValue());
    }
    return ret;
}

std::set<std::string> Manager_DB::groupAttribs(const std::string &sGroupName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING sAttribName;
    QueryInstance i = sqlConnector->query("SELECT `f_attribname` FROM vauth_v3_attribsgroups WHERE `f_groupname`=:groupname;",
                                          { {":groupname",new Memory::Abstract::STRING(sGroupName)} },
                                          { &sAttribName });
    while (i.ok && i.query->step())
    {
        ret.insert(sAttribName.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

std::set<std::string> Manager_DB::groupAccounts(const std::string &sGroupName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING sUserName;
    QueryInstance i = sqlConnector->query("SELECT `f_username` FROM vauth_v3_groupsaccounts WHERE `f_groupname`=:groupname;",
                                          { {":groupname",new Memory::Abstract::STRING(sGroupName)} },
                                          { &sUserName });
    while (i.ok && i.query->step())
    {
        ret.insert(sUserName.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

