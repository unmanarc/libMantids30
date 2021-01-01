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

bool Manager_DB::accountAdd(const std::string &accountName, const Secret &passData, const std::string &email, const std::string &accountDescription, const std::string &extraData, time_t expirationDate, bool enabled, bool confirmed, bool superuser)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("INSERT INTO vauth_v2_accounts (name,email,description,extraData,superuser) "
                               "VALUES(:name,:email,:description,:extraData,:superuser);",
                               {
                                   {":name",Abstract::STRING(accountName)},
                                   {":email",Abstract::STRING(email)},
                                   {":description",Abstract::STRING(accountDescription)},
                                   {":extraData",Abstract::STRING(extraData)},
                                   {":superuser",Abstract::BOOL(superuser)}
                               }
                               )
            &&
            sqlConnector->query("INSERT INTO vauth_v2_account_activation (`account`,`enabled`,`expiration`,`confirmed`,`confirmationToken`) "
                                "VALUES(:account,:enabled,:expiration,:confirmed,:confirmationToken);",
                                {
                                    {":account",Abstract::STRING(accountName)},
                                    {":enabled",Abstract::BOOL(enabled)},
                                    {":expiration",Abstract::DATETIME(expirationDate)},
                                    {":confirmed",Abstract::BOOL(confirmed)},
                                    {":confirmationToken",Abstract::STRING(genRandomConfirmationToken())}
                                }
                                )
            &&
            sqlConnector->query("INSERT INTO vauth_v2_account_passwords "
                                "(`index`,`account`,`hash`,`expiration`,`function`,`salt`,`forcedExpiration`,`steps`)"
                                " VALUES"
                                "(0,:account,:hash,:expiration,:function,:salt,:forcedExpiration,:steps);",
                                {
                                    {":account",Abstract::STRING(accountName)},
                                    {":hash",Abstract::STRING(passData.hash)},
                                    {":expiration",Abstract::DATETIME(passData.expiration)},
                                    {":function",Abstract::INT32(passData.passwordFunction)},
                                    {":salt",Abstract::STRING(CX2::Helpers::Encoders::toHex(passData.ssalt,4))},
                                    {":forcedExpiration",Abstract::BOOL(passData.forceExpiration)},
                                    {":steps",Abstract::UINT32(passData.gAuthSteps)}
                                }
                                );

}

std::string Manager_DB::accountConfirmationToken(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING token;
    QueryInstance i = sqlConnector->query("SELECT confirmationToken FROM vauth_v2_account_activation WHERE account=:account LIMIT 1;",
                                          { {":account",Memory::Abstract::STRING(accountName)} },
                                          { &token });
    if (i.ok && i.query->step())
    {
        return token.getValue();
    }
    return "";
}

bool Manager_DB::accountRemove(const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("DELETE FROM vauth_v2_accounts WHERE name=:name;",
                               {
                                   {":name",Abstract::STRING(accountName)}
                               });

}

bool Manager_DB::accountExist(const std::string &accountName)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(mutex);

    QueryInstance i = sqlConnector->query("SELECT enabled FROM vauth_v2_accounts WHERE name=:name LIMIT 1;",
                                          {{":name",Memory::Abstract::STRING(accountName)}},
                                          { });
    if (i.ok && i.query->step())
    {
        ret = true;
    }
    return ret;
}

bool Manager_DB::accountDisable(const std::string &accountName, bool disabled)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v2_account_activation SET enabled=:enabled WHERE name=:name;",
                               {
                                   {":enabled",Abstract::BOOL(!disabled)},
                                   {":name",Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::accountConfirm(const std::string &accountName, const std::string &confirmationToken)
{
    Threads::Sync::Lock_RW lock(mutex);

    Abstract::STRING token;
    QueryInstance i = sqlConnector->query("SELECT confirmationToken FROM vauth_v2_account_activation WHERE account=:account LIMIT 1;",
                                          { {":account",Memory::Abstract::STRING(accountName)} },
                                          { &token });

    if (i.ok && i.query->step())
    {
        if (!token.getValue().empty() && token.getValue() == confirmationToken)
        {
            return sqlConnector->query("UPDATE vauth_v2_account_activation SET confirmed='1' WHERE account=:account;",
                                       {
                                           {":account",Abstract::STRING(accountName)}
                                       });
        }
    }
    return false;
}

bool Manager_DB::accountChangeSecret(const std::string &accountName, const Secret &passwordData, uint32_t passIndex)
{
    Threads::Sync::Lock_RW lock(mutex);


    return  sqlConnector->query("DELETE FROM vauth_v2_account_passwords WHERE account=:account and `index`=:index",
                                {
                                    {":account",Abstract::STRING(accountName)},
                                    {":index",Abstract::UINT32(passIndex)}
                                })
            &&
            sqlConnector->query("INSERT INTO vauth_v2_account_passwords "
                                "(`index`,`account`,`hash`,`expiration`,`function`,`salt`,`forcedExpiration`,`steps`) "
                                "VALUES"
                                "(:index,:account,:hash,:expiration,:function,:salt,:forcedExpiration,:steps);",
                                {
                                    {":index",Abstract::UINT32(passIndex)},
                                    {":account",Abstract::STRING(accountName)},
                                    {":hash",Abstract::STRING(passwordData.hash)},
                                    {":expiration",Abstract::DATETIME(passwordData.expiration)},
                                    {":function",Abstract::UINT32(passwordData.passwordFunction)},
                                    {":salt",Abstract::STRING(CX2::Helpers::Encoders::toHex(passwordData.ssalt,4))},
                                    {":forcedExpiration",Abstract::BOOL(passwordData.forceExpiration)},
                                    {":function",Abstract::UINT32(passwordData.gAuthSteps)}
                                });

}

bool Manager_DB::accountChangeDescription(const std::string &accountName, const std::string &description)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v2_accounts SET description=:description WHERE name=:accountName;",
                               {
                                   {":description",Abstract::STRING(description)},
                                   {":accountName",Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::accountChangeEmail(const std::string &accountName, const std::string &email)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("UPDATE vauth_v2_accounts SET email=:email WHERE name=:accountName;",
                               {
                                   {":email",Abstract::STRING(email)},
                                   {":accountName",Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::accountChangeExtraData(const std::string &accountName, const std::string &extraData)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v2_accounts SET extraData=:extraData WHERE name=:accountName;",
                               {
                                   {":extraData",Abstract::STRING(extraData)},
                                   {":accountName",Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::accountChangeExpiration(const std::string &accountName, time_t expiration)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("UPDATE vauth_v2_account_activation SET expiration=:expiration WHERE account=:accountName;",
                               {
                                   {":expiration",Abstract::DATETIME(expiration)},
                                   {":accountName",Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::isAccountDisabled(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL enabled;
    QueryInstance i = sqlConnector->query("SELECT enabled FROM vauth_v2_account_activation WHERE account=:account LIMIT 1;",
                                          { {":account",Memory::Abstract::STRING(accountName)} },
                                          { &enabled });

    if (i.ok && i.query->step())
    {
        return !enabled.getValue();
    }
    return true;
}

bool Manager_DB::isAccountConfirmed(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL confirmed;
    QueryInstance i = sqlConnector->query("SELECT confirmed FROM vauth_v2_account_activation WHERE account=:account LIMIT 1;",
                                          { {":account",Memory::Abstract::STRING(accountName)} },
                                          { &confirmed });

    if (i.ok && i.query->step())
    {
        return confirmed.getValue();
    }
    return false;
}

bool Manager_DB::isAccountSuperUser(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL superuser;
    QueryInstance i = sqlConnector->query("SELECT superuser FROM vauth_v2_accounts WHERE name=:name LIMIT 1;",
                                          { {":name",Memory::Abstract::STRING(accountName)} },
                                          { &superuser });

    if (i.ok && i.query->step())
    {
        return superuser.getValue();
    }
    return false;
}

std::string Manager_DB::accountDescription(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING description;
    QueryInstance i = sqlConnector->query("SELECT description FROM vauth_v2_accounts WHERE name=:name LIMIT 1;",
                                          { {":name",Memory::Abstract::STRING(accountName)} },
                                          { &description });

    if (i.ok && i.query->step())
    {
        return description.getValue();
    }
    return "";
}

std::string Manager_DB::accountEmail(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING email;
    QueryInstance i = sqlConnector->query("SELECT email FROM vauth_v2_accounts WHERE name=:name LIMIT 1;",
                                          { {":name",Memory::Abstract::STRING(accountName)} },
                                          { &email });

    if (i.ok && i.query->step())
    {
        return email.getValue();
    }
    return "";
}

std::string Manager_DB::accountExtraData(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING extraData;
    QueryInstance i = sqlConnector->query("SELECT extraData FROM vauth_v2_accounts WHERE name=:name LIMIT 1;",
                                          { {":name",Memory::Abstract::STRING(accountName)} },
                                          { &extraData });

    if (i.ok && i.query->step())
    {
        return extraData.getValue();
    }
    return "";
}

time_t Manager_DB::accountExpirationDate(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::DATETIME expiration;
    QueryInstance i = sqlConnector->query("SELECT expiration FROM vauth_v2_account_activation WHERE account=:name LIMIT 1;",
                                          { {":name",Memory::Abstract::STRING(accountName)} },
                                          { &expiration });

    if (i.ok && i.query->step())
    {
        return expiration.getValue();
    }
    return std::numeric_limits<time_t>::max();
}

std::set<std::string> Manager_DB::accountsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING name;
    QueryInstance i = sqlConnector->query("SELECT name FROM vauth_v2_accounts;",
                                          { },
                                          { &name });
    while (i.ok && i.query->step())
    {
        ret.insert(name.getValue());
    }

    return ret;
}

std::set<std::string> Manager_DB::accountGroups(const std::string &accountName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING group;
    QueryInstance i = sqlConnector->query("SELECT group_name FROM vauth_v2_groups_accounts WHERE  account_name=:account;",
                                          { {":account",Memory::Abstract::STRING(accountName)} },
                                          { &group });
    while (i.ok && i.query->step())
    {
        ret.insert(group.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

std::set<std::string> Manager_DB::accountDirectAttribs(const std::string &accountName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING attrib;
    QueryInstance i = sqlConnector->query("SELECT attrib_name FROM vauth_v2_attribs_accounts WHERE account_name=:account;",
                                          { {":account",Memory::Abstract::STRING(accountName)} },
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

    QueryInstance i = sqlConnector->query("SELECT superuser FROM vauth_v2_accounts WHERE superuser=:superuser LIMIT 1;",
                                          { {":superuser",Memory::Abstract::BOOL(true)} },
                                          { });

    if (i.ok && i.query->step())
        return true;

    return false;
}

Secret Manager_DB::retrieveSecret(const std::string &accountName, uint32_t passIndex, bool *found)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::UINT32 steps,function;
    Abstract::BOOL forcedExpiration;
    Abstract::DATETIME expiration;
    Abstract::STRING salt,hash;

    Secret ret;
    QueryInstance i = sqlConnector->query("SELECT steps,forcedExpiration,function,expiration,salt,hash FROM vauth_v2_account_passwords "
                                          "WHERE `account`=:account AND `index`=:index LIMIT 1;",
                                          { {":account",Memory::Abstract::STRING(accountName)},
                                            {":index",Memory::Abstract::UINT32(passIndex)}
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

