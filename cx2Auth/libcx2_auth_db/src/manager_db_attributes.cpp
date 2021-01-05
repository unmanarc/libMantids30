#include "manager_db.h"
#include <cx2_thr_mutex/lock_shared.h>

#include <cx2_mem_vars/a_string.h>

using namespace CX2::Authentication;
using namespace CX2::Memory;
using namespace CX2::Database;

bool Manager_DB::attribAdd(const std::string &sAttribName, const std::string &sAttribDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("INSERT INTO vauth_v3_attribs (`attribname`,`description`) VALUES(:attribname,:description);",
                               {
                                   {":attribname",new Abstract::STRING(sAttribName)},
                                   {":description",new Abstract::STRING(sAttribDescription)}
                               });
}

bool Manager_DB::attribRemove(const std::string &sAttribName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("DELETE FROM vauth_v3_attribs WHERE `attribname`=:attribname;",
                               {
                                   {":attribname",new Abstract::STRING(sAttribName)}
                               });
}

bool Manager_DB::attribExist(const std::string &sAttribName)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(mutex);

    QueryInstance i = sqlConnector->query("SELECT `description` FROM vauth_v3_attribs WHERE `attribname`=:attribname LIMIT 1;",
                                          {{":attribname",new Memory::Abstract::STRING(sAttribName)}},
                                          { });
    if (i.ok && i.query->step())
    {
        ret = true;
    }
    return ret;
}

bool Manager_DB::attribGroupAdd(const std::string &sAttribName, const std::string &sGroupName)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("INSERT INTO vauth_v3_attribsgroups (`f_attribname`,`f_groupname`) VALUES(:attribname,:groupname);",
                               {
                                   {":attribname",new Abstract::STRING(sAttribName)},
                                   {":groupname",new Abstract::STRING(sGroupName)}
                               });
}

bool Manager_DB::attribGroupRemove(const std::string &sAttribName, const std::string &sGroupName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();
    ret = sqlConnector->query("DELETE FROM vauth_v3_attribsgroups WHERE `f_attribname`=:attribname AND `f_groupname`=:groupname;",
                              {
                                  {":attribname",new Abstract::STRING(sAttribName)},
                                  {":groupname",new Abstract::STRING(sGroupName)}
                              });
    if (lock) mutex.unlock();
    return ret;
}

bool Manager_DB::attribAccountAdd(const std::string &sAttribName, const std::string &sUserName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("INSERT INTO vauth_v3_attribsaccounts (`f_attribname`,`f_username`) VALUES(:attribname,:username);",
                               {
                                   {":attribname",new Abstract::STRING(sAttribName)},
                                   {":username",new Abstract::STRING(sUserName)}
                               });
}

bool Manager_DB::attribAccountRemove(const std::string &sAttribName, const std::string &sUserName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();
    ret = sqlConnector->query("DELETE FROM vauth_v3_attribsaccounts WHERE `f_attribname`=:attribname AND `f_username`=:username;",
                              {
                                  {":attribname",new Abstract::STRING(sAttribName)},
                                  {":username",new Abstract::STRING(sUserName)}
                              });
    if (lock) mutex.unlock();
    return ret;
}

bool Manager_DB::attribChangeDescription(const std::string &sAttribName, const std::string &attribDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v3_attribs SET `description`=:description WHERE `attribname`=:attribname;",
                               {
                                   {":attribname",new Abstract::STRING(sAttribName)},
                                   {":description",new Abstract::STRING(attribDescription)}
                               });
}

std::string Manager_DB::attribDescription(const std::string &sAttribName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING description;
    QueryInstance i = sqlConnector->query("SELECT `description` FROM vauth_v3_attribs WHERE `attribname`=:attribname LIMIT 1;",
                                          {{":attribname",new Memory::Abstract::STRING(sAttribName)}},
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

    Abstract::STRING sAttribName;
    QueryInstance i = sqlConnector->query("SELECT `attribname` FROM vauth_v3_attribs;",
                                          {},
                                          { &sAttribName });
    while (i.ok && i.query->step())
    {
        ret.insert(sAttribName.getValue());
    }
    return ret;
}

std::set<std::string> Manager_DB::attribGroups(const std::string &sAttribName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING gruopName;
    QueryInstance i = sqlConnector->query("SELECT `f_groupname` FROM vauth_v3_attribsgroups WHERE `f_attribname`=:attribname;",
                                          { {":attribname",new Memory::Abstract::STRING(sAttribName)} },
                                          { &gruopName });
    while (i.ok && i.query->step())
    {
        ret.insert(gruopName.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

std::set<std::string> Manager_DB::attribAccounts(const std::string &sAttribName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();


    Abstract::STRING sUserName;
    QueryInstance i = sqlConnector->query("SELECT `f_username` FROM vauth_v3_attribsaccounts WHERE `f_attribname`=:attribname;",
                                          { {":attribname",new Memory::Abstract::STRING(sAttribName)} },
                                          { &sUserName });
    while (i.ok && i.query->step())
    {
        ret.insert(sUserName.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

bool Manager_DB::accountValidateDirectAttribute(const std::string &sUserName, const std::string &sAttribName)
{
    Threads::Sync::Lock_RD lock(mutex);

    QueryInstance i = sqlConnector->query("SELECT `f_username` FROM vauth_v3_attribsaccounts WHERE `f_attribname`=:attribname AND `f_username`=:username;",
                                          { {":attribname",new Memory::Abstract::STRING(sAttribName)},
                                            {":username",new Memory::Abstract::STRING(sUserName)}
                                          },
                                          { });
    return (i.ok && i.query->step());
}
