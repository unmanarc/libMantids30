#include "manager_db.h"

#include <limits>
#include <cx2_thr_mutex/lock_shared.h>

#include <cx2_mem_vars/a_string.h>
#include <cx2_mem_vars/a_datetime.h>
#include <cx2_mem_vars/a_bool.h>
#include <cx2_mem_vars/a_int32.h>
#include <cx2_mem_vars/a_uint32.h>
#include <cx2_mem_vars/a_var.h>

using namespace CX2::Authentication;
using namespace CX2::Memory;
using namespace CX2::Database;

std::set<uint32_t> Manager_DB::passIndexesUsedByAccount(const std::string &sAccountName)
{
    std::set<uint32_t> r;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::UINT32 idx;
    QueryInstance i = sqlConnector->query("SELECT `f_secretIndex` FROM vauth_v3_accountsecrets WHERE `f_userName`=:f_userName;",
                                          {  {":f_userName",             new Memory::Abstract::STRING(sAccountName)} },
                                          { &idx });

    while (i.ok && i.query->step())
    {
        r.insert(idx.getValue());
    }

    return r;
}

std::set<uint32_t> Manager_DB::passIndexesRequiredForLogin()
{
    std::set<uint32_t> r;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::UINT32 idx;
    QueryInstance i = sqlConnector->query("SELECT `index` FROM vauth_v3_secretsindexs WHERE `loginRequired`=:loginRequired;",
                                          {  {":loginRequired",             new Memory::Abstract::BOOL(true)} },
                                          { &idx });

    while (i.ok && i.query->step())
    {
        r.insert(idx.getValue());
    }

    return r;
}

bool Manager_DB::passIndexAdd(const uint32_t &passIndex, const std::string &description, const bool &loginRequired)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("INSERT INTO vauth_v3_secretsindexs (`index`,`indexDescription`,`loginRequired`) "
                        "VALUES(:index,:indexDescription,:loginRequired);",
                               {
                                   {":index",             new Memory::Abstract::UINT32(passIndex)},
                                   {":indexDescription",  new Memory::Abstract::STRING(description)},
                                   {":loginRequired",     new Memory::Abstract::BOOL(loginRequired)}
                               });
}

bool Manager_DB::passIndexModify(const uint32_t &passIndex, const std::string &description, const bool &loginRequired)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v3_secretsindexs SET `loginRequired`=:loginRequired, `indexDescription`=:indexDescription WHERE `index`=:index;",
                               {
                                   {":index",             new Memory::Abstract::UINT32(passIndex)},
                                   {":indexDescription",  new Memory::Abstract::STRING(description)},
                                   {":loginRequired",     new Memory::Abstract::BOOL(loginRequired)}
                               });
}

bool Manager_DB::passIndexDelete(const uint32_t &passIndex)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("DELETE FROM vauth_v3_secretsindexs WHERE `index`=:index;",
                               {
                                   {":index", new Memory::Abstract::UINT32(passIndex)}
                               });
}

std::string Manager_DB::passIndexDescription(const uint32_t &passIndex)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING description;
    QueryInstance i = sqlConnector->query("SELECT `indexDescription` FROM vauth_v3_secretsindexs WHERE `index`=:index LIMIT 1;",
                                          {  {":index",             new Memory::Abstract::UINT32(passIndex)} },
                                          { &description });

    if (i.ok && i.query->step())
    {
        return description.getValue();
    }
    return "";
}

bool Manager_DB::passIndexLoginRequired(const uint32_t &passIndex)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL loginRequired;
    QueryInstance i = sqlConnector->query("SELECT `loginRequired` FROM vauth_v3_secretsindexs WHERE `index`=:index LIMIT 1;",
                                          {  {":index",             new Memory::Abstract::UINT32(passIndex)} },
                                          { &loginRequired });

    if (i.ok && i.query->step())
    {
        return loginRequired.getValue();
    }

    return false;
}
