#include "manager_db.h"

#include <limits>
#include <Mantids29/Threads/lock_shared.h>

#include <Mantids29/Memory/a_string.h>
#include <Mantids29/Memory/a_datetime.h>
#include <Mantids29/Memory/a_bool.h>
#include <Mantids29/Memory/a_int32.h>
#include <Mantids29/Memory/a_uint32.h>
#include <Mantids29/Memory/a_var.h>

using namespace Mantids29::Authentication;
using namespace Mantids29::Memory;
using namespace Mantids29::Database;

std::set<uint32_t> Manager_DB::passIndexesUsedByAccount(const std::string &accountName)
{
    std::set<uint32_t> r;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::UINT32 idx;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_secretIndex` FROM vauth_v4_accountsecrets WHERE `f_userName`=:f_userName;",
                                          {  {":f_userName",             new Memory::Abstract::STRING(accountName)} },
                                          { &idx });

    while (i->getResultsOK() && i->query->step())
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
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `index` FROM vauth_v4_secretsindices WHERE `isLoginRequired`=:loginRequired;",
                                          {  {":loginRequired",             new Memory::Abstract::BOOL(true)} },
                                          { &idx });

    while (i->getResultsOK() && i->query->step())
    {
        r.insert(idx.getValue());
    }

    return r;
}

bool Manager_DB::passIndexAdd(const uint32_t &passIndex, const std::string &description, const bool &loginRequired)
{
    Threads::Sync::Lock_RW lock(mutex);

    return m_sqlConnector->query("INSERT INTO vauth_v4_secretsindices (`index`,`indexDescription`,`isLoginRequired`) "
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
    return m_sqlConnector->query("UPDATE vauth_v4_secretsindices SET `isLoginRequired`=:loginRequired, `indexDescription`=:indexDescription WHERE `index`=:index;",
                               {
                                   {":index",             new Memory::Abstract::UINT32(passIndex)},
                                   {":indexDescription",  new Memory::Abstract::STRING(description)},
                                   {":loginRequired",     new Memory::Abstract::BOOL(loginRequired)}
                               });
}

bool Manager_DB::passIndexDelete(const uint32_t &passIndex)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("DELETE FROM vauth_v4_secretsindices WHERE `index`=:index;",
                               {
                                   {":index", new Memory::Abstract::UINT32(passIndex)}
                               });
}

std::string Manager_DB::passIndexDescription(const uint32_t &passIndex)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING description;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `indexDescription` FROM vauth_v4_secretsindices WHERE `index`=:index LIMIT 1;",
                                          {  {":index",             new Memory::Abstract::UINT32(passIndex)} },
                                          { &description });

    if (i->getResultsOK() && i->query->step())
    {
        return description.getValue();
    }
    return "";
}

bool Manager_DB::passIndexLoginRequired(const uint32_t &passIndex)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL loginRequired;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `isLoginRequired` FROM vauth_v4_secretsindices WHERE `index`=:index LIMIT 1;",
                                          {  {":index",             new Memory::Abstract::UINT32(passIndex)} },
                                          { &loginRequired });

    if (i->getResultsOK() && i->query->step())
    {
        return loginRequired.getValue();
    }

    return false;
}
