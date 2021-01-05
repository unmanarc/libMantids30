#include "manager_db.h"

#include <limits>
#include <cx2_thr_mutex/lock_shared.h>

#include <cx2_mem_vars/a_string.h>
#include <cx2_mem_vars/a_datetime.h>
#include <cx2_mem_vars/a_bool.h>
#include <cx2_mem_vars/a_int32.h>
#include <cx2_mem_vars/a_uint32.h>

using namespace CX2::Authentication;
using namespace CX2::Memory;
using namespace CX2::Database;

bool Manager_DB::accountAdd(const std::string &sUserName, const Secret &secretData, const std::string &email, const std::string &accountDescription, const std::string &extraData, time_t expirationDate, bool enabled, bool confirmed, bool superuser)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("INSERT INTO vauth_v3_accounts (`username`,`email`,`description`,`extraData`,`superuser`,`enabled`,`expiration`,`confirmed`) "
                               "VALUES(:username,:email,:description,:extraData,:superuser,:enabled,:expiration,:confirmed);",
                               {
                                   {":username",new Abstract::STRING(sUserName)},
                                   {":email",new Abstract::STRING(email)},
                                   {":description",new Abstract::STRING(accountDescription)},
                                   {":extraData",new Abstract::STRING(extraData)},
                                   {":superuser",new Abstract::BOOL(superuser)},
                                   {":enabled",new Abstract::BOOL(enabled)},
                                   {":expiration",new Abstract::DATETIME(expirationDate)},
                                   {":confirmed",new Abstract::BOOL(confirmed)}
                               }
                               )
            &&
            sqlConnector->query("INSERT INTO vauth_v3_accountactivationtokens (`f_username`,`confirmationToken`) "
                                "VALUES(:account,:confirmationToken);",
                                {
                                    {":account",new Abstract::STRING(sUserName)},
                                    {":confirmationToken",new Abstract::STRING(genRandomConfirmationToken())}
                                }
                                )
            &&
            sqlConnector->query("INSERT INTO vauth_v3_accountsecrets "
                                "(`index`,`f_username`,`hash`,`expiration`,`function`,`salt`,`forcedExpiration`,`steps`)"
                                " VALUES"
                                "(0,:account,:hash,:expiration,:function,:salt,:forcedExpiration,:steps);",
                                {
                                    {":account",new Abstract::STRING(sUserName)},
                                    {":hash",new Abstract::STRING(secretData.hash)},
                                    {":expiration",new Abstract::DATETIME(secretData.expiration)},
                                    {":function",new Abstract::INT32(secretData.passwordFunction)},
                                    {":salt",new Abstract::STRING(CX2::Helpers::Encoders::toHex(secretData.ssalt,4))},
                                    {":forcedExpiration",new Abstract::BOOL(secretData.forceExpiration)},
                                    {":steps",new Abstract::UINT32(secretData.gAuthSteps)}
                                }
                                );

}

std::string Manager_DB::accountConfirmationToken(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING token;
    QueryInstance i = sqlConnector->query("SELECT confirmationToken FROM vauth_v3_accountactivationtokens WHERE `f_username`=:username LIMIT 1;",
                                          { {":username",new Memory::Abstract::STRING(sUserName)} },
                                          { &token });
    if (i.ok && i.query->step())
    {
        return token.getValue();
    }
    return "";
}

bool Manager_DB::accountRemove(const std::string &sUserName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("DELETE FROM vauth_v3_accounts WHERE `username`=:username;",
                               {
                                   {":username",new Abstract::STRING(sUserName)}
                               });

}

bool Manager_DB::accountExist(const std::string &sUserName)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(mutex);

    QueryInstance i = sqlConnector->query("SELECT `enabled` FROM vauth_v3_accounts WHERE `username`=:username LIMIT 1;",
                                          {{":username",new Memory::Abstract::STRING(sUserName)}},
                                          { });
    if (i.ok && i.query->step())
    {
        ret = true;
    }
    return ret;
}

bool Manager_DB::accountDisable(const std::string &sUserName, bool disabled)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v3_accounts SET `enabled`=:enabled WHERE `username`=:username;",
                               {
                                   {":enabled",new Abstract::BOOL(!disabled)},
                                   {":username",new Abstract::STRING(sUserName)}
                               });
}

bool Manager_DB::accountConfirm(const std::string &sUserName, const std::string &confirmationToken)
{
    Threads::Sync::Lock_RW lock(mutex);

    Abstract::STRING token;
    QueryInstance i = sqlConnector->query("SELECT `confirmationToken` FROM vauth_v3_accountactivationtokens WHERE `f_username`=:username LIMIT 1;",
                                          { {":username",new Memory::Abstract::STRING(sUserName)} },
                                          { &token });

    if (i.ok && i.query->step())
    {
        if (!token.getValue().empty() && token.getValue() == confirmationToken)
        {
            return sqlConnector->query("UPDATE vauth_v3_accounts SET `confirmed`='1' WHERE `username`=:username;",
                                       {
                                           {":username",new Abstract::STRING(sUserName)}
                                       });
        }
    }
    return false;
}

bool Manager_DB::accountChangeSecret(const std::string &sUserName, const Secret &passwordData, uint32_t passIndex)
{
    Threads::Sync::Lock_RW lock(mutex);


    return  sqlConnector->query("DELETE FROM vauth_v3_accountsecrets WHERE `f_username`=:username and `index`=:index",
                                {
                                    {":username",new Abstract::STRING(sUserName)},
                                    {":index",new Abstract::UINT32(passIndex)}
                                })
            &&
            sqlConnector->query("INSERT INTO vauth_v3_accountsecrets "
                                "(`index`,`f_username`,`hash`,`expiration`,`function`,`salt`,`forcedExpiration`,`steps`) "
                                "VALUES"
                                "(:index,:account,:hash,:expiration,:function,:salt,:forcedExpiration,:steps);",
                                {
                                    {":index",new Abstract::UINT32(passIndex)},
                                    {":account",new Abstract::STRING(sUserName)},
                                    {":hash",new Abstract::STRING(passwordData.hash)},
                                    {":expiration",new Abstract::DATETIME(passwordData.expiration)},
                                    {":function",new Abstract::UINT32(passwordData.passwordFunction)},
                                    {":salt",new Abstract::STRING(CX2::Helpers::Encoders::toHex(passwordData.ssalt,4))},
                                    {":forcedExpiration",new Abstract::BOOL(passwordData.forceExpiration)},
                                    {":steps",new Abstract::UINT32(passwordData.gAuthSteps)}
                                });

}

bool Manager_DB::accountChangeDescription(const std::string &sUserName, const std::string &description)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v3_accounts SET `description`=:description WHERE `username`=:username;",
                               {
                                   {":description",new Abstract::STRING(description)},
                                   {":username",new Abstract::STRING(sUserName)}
                               });
}

bool Manager_DB::accountChangeEmail(const std::string &sUserName, const std::string &email)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("UPDATE vauth_v3_accounts SET `email`=:email WHERE `username`=:username;",
                               {
                                   {":email",new Abstract::STRING(email)},
                                   {":username",new Abstract::STRING(sUserName)}
                               });
}

bool Manager_DB::accountChangeExtraData(const std::string &sUserName, const std::string &extraData)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v3_accounts SET `extraData`=:extraData WHERE `username`=:username;",
                               {
                                   {":extraData",new Abstract::STRING(extraData)},
                                   {":username",new Abstract::STRING(sUserName)}
                               });
}

bool Manager_DB::accountChangeExpiration(const std::string &sUserName, time_t expiration)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("UPDATE vauth_v3_accounts SET `expiration`=:expiration WHERE `username`=:username;",
                               {
                                   {":expiration",new Abstract::DATETIME(expiration)},
                                   {":username",new Abstract::STRING(sUserName)}
                               });
}

bool Manager_DB::isAccountDisabled(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL enabled;
    QueryInstance i = sqlConnector->query("SELECT `enabled` FROM vauth_v3_accounts WHERE `username`=:username LIMIT 1;",
                                          { {":username",new Memory::Abstract::STRING(sUserName)} },
                                          { &enabled });

    if (i.ok && i.query->step())
    {
        return !enabled.getValue();
    }
    return true;
}

bool Manager_DB::isAccountConfirmed(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL confirmed;
    QueryInstance i = sqlConnector->query("SELECT `confirmed` FROM vauth_v3_accounts WHERE `username`=:username LIMIT 1;",
                                          { {":username",new Memory::Abstract::STRING(sUserName)} },
                                          { &confirmed });

    if (i.ok && i.query->step())
    {
        return confirmed.getValue();
    }
    return false;
}

bool Manager_DB::isAccountSuperUser(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL superuser;
    QueryInstance i = sqlConnector->query("SELECT `superuser` FROM vauth_v3_accounts WHERE `username`=:username LIMIT 1;",
                                          { {":username",new Memory::Abstract::STRING(sUserName)} },
                                          { &superuser });

    if (i.ok && i.query->step())
    {
        return superuser.getValue();
    }
    return false;
}

std::string Manager_DB::accountDescription(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING description;
    QueryInstance i = sqlConnector->query("SELECT `description` FROM vauth_v3_accounts WHERE `username`=:username LIMIT 1;",
                                          { {":username",new Memory::Abstract::STRING(sUserName)} },
                                          { &description });

    if (i.ok && i.query->step())
    {
        return description.getValue();
    }
    return "";
}

std::string Manager_DB::accountEmail(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING email;
    QueryInstance i = sqlConnector->query("SELECT `email` FROM vauth_v3_accounts WHERE `username`=:username LIMIT 1;",
                                          { {":username",new Memory::Abstract::STRING(sUserName)} },
                                          { &email });

    if (i.ok && i.query->step())
    {
        return email.getValue();
    }
    return "";
}

std::string Manager_DB::accountExtraData(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING extraData;
    QueryInstance i = sqlConnector->query("SELECT `extraData` FROM vauth_v3_accounts WHERE `username`=:username LIMIT 1;",
                                          { {":username",new Memory::Abstract::STRING(sUserName)} },
                                          { &extraData });

    if (i.ok && i.query->step())
    {
        return extraData.getValue();
    }
    return "";
}

time_t Manager_DB::accountExpirationDate(const std::string &sUserName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::DATETIME expiration;
    QueryInstance i = sqlConnector->query("SELECT `expiration` FROM vauth_v3_accounts WHERE `username`=:username LIMIT 1;",
                                          { {":username",new Memory::Abstract::STRING(sUserName)} },
                                          { &expiration });
    if (i.ok && i.query->step())
    {
        return expiration.getValue();
    }
    // If can't get this data, the account is expired:
    return 1;
}

std::set<std::string> Manager_DB::accountsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sUserName;
    QueryInstance i = sqlConnector->query("SELECT `username` FROM vauth_v3_accounts;",
                                          { },
                                          { &sUserName });
    while (i.ok && i.query->step())
    {
        ret.insert(sUserName.getValue());
    }

    return ret;
}

std::set<std::string> Manager_DB::accountGroups(const std::string &sUserName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING group;
    QueryInstance i = sqlConnector->query("SELECT `f_groupname` FROM vauth_v3_groupsaccounts WHERE `f_username`=:username;",
                                          { {":username",new Memory::Abstract::STRING(sUserName)} },
                                          { &group });
    while (i.ok && i.query->step())
    {
        ret.insert(group.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

std::set<std::string> Manager_DB::accountDirectAttribs(const std::string &sUserName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING attrib;
    QueryInstance i = sqlConnector->query("SELECT `f_attribname` FROM vauth_v3_attribsaccounts WHERE `f_username`=:username;",
                                          { {":username",new Memory::Abstract::STRING(sUserName)} },
                                          { &attrib });
    while (i.ok && i.query->step())
    {
        ret.insert(attrib.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

bool Manager_DB::superUserAccountExist()
{
    Threads::Sync::Lock_RD lock(mutex);

    QueryInstance i = sqlConnector->query("SELECT `superuser` FROM vauth_v3_accounts WHERE `superuser`=:superuser LIMIT 1;",
                                          { {":superuser",new Memory::Abstract::BOOL(true)} },
                                          { });

    if (i.ok && i.query->step())
        return true;

    return false;
}

Secret Manager_DB::retrieveSecret(const std::string &sUserName, uint32_t passIndex, bool *found)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::UINT32 steps,function;
    Abstract::BOOL forcedExpiration;
    Abstract::DATETIME expiration;
    Abstract::STRING salt,hash;

    Secret ret;
    QueryInstance i = sqlConnector->query("SELECT `steps`,`forcedExpiration`,`function`,`expiration`,`salt`,`hash` FROM vauth_v3_accountsecrets "
                                          "WHERE `f_username`=:username AND `index`=:index LIMIT 1;",
                                          { {":username",new Memory::Abstract::STRING(sUserName)},
                                            {":index",new Memory::Abstract::UINT32(passIndex)}
                                          },
                                          { &steps, &forcedExpiration, &function, &expiration, &salt, &hash });

    if (found) *found = false;
    if (i.ok && i.query->step())
    {
        if (found) *found = true;
        ret.gAuthSteps = steps.getValue();
        ret.forceExpiration = forcedExpiration.getValue();
        ret.passwordFunction = (Authentication::Function)function.getValue();
        ret.expiration = expiration.getValue();
        CX2::Helpers::Encoders::fromHex(salt.getValue(),ret.ssalt,4);
        ret.hash = hash.getValue();
    }
    return ret;
}

