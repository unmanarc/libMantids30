#include "manager_db.h"

#include <limits>
#include <mdz_thr_mutex/lock_shared.h>

#include <mdz_mem_vars/a_string.h>
#include <mdz_mem_vars/a_datetime.h>
#include <mdz_mem_vars/a_bool.h>
#include <mdz_mem_vars/a_int32.h>
#include <mdz_mem_vars/a_uint32.h>
#include <mdz_mem_vars/a_uint64.h>
#include <mdz_mem_vars/a_var.h>

using namespace Mantids::Authentication;
using namespace Mantids::Memory;
using namespace Mantids::Database;

bool Manager_DB::accountAdd(const std::string &sAccountName, const Secret &secretData, const sAccountDetails &accountDetails, time_t expirationDate, const sAccountAttribs &accountAttribs, const std::string &sCreatorAccountName)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("INSERT INTO vauth_v3_accounts (`userName`,`givenName`,`lastName`,`email`,`description`,`extraData`,`superuser`,`enabled`,`expiration`,`confirmed`,`creator`) "
                                                       "VALUES(:userName ,:givenname ,:lastname ,:email ,:description ,:extraData ,:superuser ,:enabled ,:expiration ,:confirmed ,:creator);",
                               {
                                   {":userName",new Abstract::STRING(sAccountName)},
                                   {":givenname",new Abstract::STRING(accountDetails.sGivenName)},
                                   {":lastname",new Abstract::STRING(accountDetails.sLastName)},
                                   {":email",new Abstract::STRING(accountDetails.sEmail)},
                                   {":description",new Abstract::STRING(accountDetails.sDescription)},
                                   {":extraData",new Abstract::STRING(accountDetails.sExtraData)},
                                   {":superuser",new Abstract::BOOL(accountAttribs.superuser)},
                                   {":enabled",new Abstract::BOOL(accountAttribs.enabled)},
                                   {":expiration",new Abstract::DATETIME(expirationDate)},
                                   {":confirmed",new Abstract::BOOL(accountAttribs.confirmed)},
                                   {":creator", sCreatorAccountName.empty() ? new Abstract::Var() /* null */ : new Abstract::STRING(accountDetails.sExtraData)}
                               }
                               )
            &&
            sqlConnector->query("INSERT INTO vauth_v3_accountactivationtokens (`f_userName`,`confirmationToken`) "
                                "VALUES(:account,:confirmationToken);",
                                {
                                    {":account",new Abstract::STRING(sAccountName)},
                                    {":confirmationToken",new Abstract::STRING(genRandomConfirmationToken())}
                                }
                                )
            &&
            sqlConnector->query("INSERT INTO vauth_v3_accountsecrets "
                                "(`f_secretIndex`,`f_userName`,`hash`,`expiration`,`function`,`salt`,`forcedExpiration`,`steps`)"
                                " VALUES"
                                "('0',:account,:hash,:expiration,:function,:salt,:forcedExpiration,:steps);",
                                {
                                    {":account",new Abstract::STRING(sAccountName)},
                                    {":hash",new Abstract::STRING(secretData.hash)},
                                    {":expiration",new Abstract::DATETIME(secretData.expiration)},
                                    {":function",new Abstract::INT32(secretData.passwordFunction)},
                                    {":salt",new Abstract::STRING(Mantids::Helpers::Encoders::toHex(secretData.ssalt,4))},
                                    {":forcedExpiration",new Abstract::BOOL(secretData.forceExpiration)},
                                    {":steps",new Abstract::UINT32(secretData.gAuthSteps)}
                                }
                                );

}

std::string Manager_DB::accountConfirmationToken(const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING token;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT confirmationToken FROM vauth_v3_accountactivationtokens WHERE `f_userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &token });
    if (i->ok && i->query->step())
    {
        return token.getValue();
    }
    return "";
}

bool Manager_DB::accountRemove(const std::string &sAccountName)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (isThereAnotherSuperUser(sAccountName))
    {
        return sqlConnector->query("DELETE FROM vauth_v3_accounts WHERE `userName`=:userName;",
                                   {
                                       {":userName",new Abstract::STRING(sAccountName)}
                                   });
    }
    return false;
}

bool Manager_DB::accountExist(const std::string &sAccountName)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(mutex);

    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `enabled` FROM vauth_v3_accounts WHERE `userName`=:userName LIMIT 1;",
                                          {{":userName",new Memory::Abstract::STRING(sAccountName)}},
                                          { });
    if (i->ok && i->query->step())
    {
        ret = true;
    }
    return ret;
}

bool Manager_DB::accountDisable(const std::string &sAccountName, bool disabled)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (disabled==true && !isThereAnotherSuperUser(sAccountName))
    {
        return false;
    }

    return sqlConnector->query("UPDATE vauth_v3_accounts SET `enabled`=:enabled WHERE `userName`=:userName;",
                               {
                                   {":enabled",new Abstract::BOOL(!disabled)},
                                   {":userName",new Abstract::STRING(sAccountName)}
                               });
}

bool Manager_DB::accountConfirm(const std::string &sAccountName, const std::string &confirmationToken)
{
    Threads::Sync::Lock_RW lock(mutex);

    Abstract::STRING token;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `confirmationToken` FROM vauth_v3_accountactivationtokens WHERE `f_userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &token });

    if (i->ok && i->query->step())
    {
        if (!token.getValue().empty() && token.getValue() == confirmationToken)
        {
            return sqlConnector->query("UPDATE vauth_v3_accounts SET `confirmed`='1' WHERE `userName`=:userName;",
                                       {
                                           {":userName",new Abstract::STRING(sAccountName)}
                                       });
        }
    }
    return false;
}

bool Manager_DB::accountChangeSecret(const std::string &sAccountName, const Secret &passwordData, uint32_t passIndex)
{
    Threads::Sync::Lock_RW lock(mutex);

    // Destroy (if exist).
    sqlConnector->query("DELETE FROM vauth_v3_accountsecrets WHERE `f_userName`=:userName and `f_secretIndex`=:index",
                        {
                            {":userName",new Abstract::STRING(sAccountName)},
                            {":index",new Abstract::UINT32(passIndex)}
                        });

    return sqlConnector->query("INSERT INTO vauth_v3_accountsecrets "
                               "(`f_secretIndex`,`f_userName`,`hash`,`expiration`,`function`,`salt`,`forcedExpiration`,`steps`) "
                               "VALUES"
                               "(:index,:account,:hash,:expiration,:function,:salt,:forcedExpiration,:steps);",
                               {
                                   {":index",new Abstract::UINT32(passIndex)},
                                   {":account",new Abstract::STRING(sAccountName)},
                                   {":hash",new Abstract::STRING(passwordData.hash)},
                                   {":expiration",new Abstract::DATETIME(passwordData.expiration)},
                                   {":function",new Abstract::UINT32(passwordData.passwordFunction)},
                                   {":salt",new Abstract::STRING(Mantids::Helpers::Encoders::toHex(passwordData.ssalt,4))},
                                   {":forcedExpiration",new Abstract::BOOL(passwordData.forceExpiration)},
                                   {":steps",new Abstract::UINT32(passwordData.gAuthSteps)}
                               });

}

bool Manager_DB::accountChangeDescription(const std::string &sAccountName, const std::string &description)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v3_accounts SET `description`=:description WHERE `userName`=:userName;",
                               {
                                   {":description",new Abstract::STRING(description)},
                                   {":userName",new Abstract::STRING(sAccountName)}
                               });
}

bool Manager_DB::accountChangeGivenName(const std::string &sAccountName, const std::string &sGivenName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v3_accounts SET `givenName`=:givenname WHERE `userName`=:userName;",
                               {
                                   {":givenname",new Abstract::STRING(sGivenName)},
                                   {":userName",new Abstract::STRING(sAccountName)}
                               });
}

bool Manager_DB::accountChangeLastName(const std::string &sAccountName, const std::string &sLastName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v3_accounts SET `lastName`=:lastname WHERE `userName`=:userName;",
                               {
                                   {":lastname",new Abstract::STRING(sLastName)},
                                   {":userName",new Abstract::STRING(sAccountName)}
                               });
}

bool Manager_DB::accountChangeEmail(const std::string &sAccountName, const std::string &email)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("UPDATE vauth_v3_accounts SET `email`=:email WHERE `userName`=:userName;",
                               {
                                   {":email",new Abstract::STRING(email)},
                                   {":userName",new Abstract::STRING(sAccountName)}
                               });
}

bool Manager_DB::accountChangeExtraData(const std::string &sAccountName, const std::string &extraData)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v3_accounts SET `extraData`=:extraData WHERE `userName`=:userName;",
                               {
                                   {":extraData",new Abstract::STRING(extraData)},
                                   {":userName",new Abstract::STRING(sAccountName)}
                               });
}

bool Manager_DB::accountChangeExpiration(const std::string &sAccountName, time_t expiration)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("UPDATE vauth_v3_accounts SET `expiration`=:expiration WHERE `userName`=:userName;",
                               {
                                   {":expiration",new Abstract::DATETIME(expiration)},
                                   {":userName",new Abstract::STRING(sAccountName)}
                               });
}

sAccountAttribs Manager_DB::accountAttribs(const std::string &sAccountName)
{
    sAccountAttribs r;

    Abstract::BOOL enabled,confirmed,superuser;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `enabled`,`confirmed`,`superuser` FROM vauth_v3_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &enabled,&confirmed,&superuser});

    if (i->ok && i->query->step())
    {
        r.enabled = enabled.getValue();
        r.confirmed = confirmed.getValue();
        r.superuser = superuser.getValue();
    }


    return r;
}

bool Manager_DB::accountChangeGroupSet(const std::string &sAccountName, const std::set<std::string> &groupSet)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (!sqlConnector->query("DELETE FROM vauth_v3_groupsaccounts WHERE `f_userName`=:userName;",
    {
        {":userName",new Abstract::STRING(sAccountName)}
    }))
        return false;

    for (const auto & group : groupSet)
    {
        if (!sqlConnector->query("INSERT INTO vauth_v3_groupsaccounts (`f_groupName`,`f_userName`) VALUES(:groupName,:userName);",
        {
            {":groupName",new Abstract::STRING(group)},
            {":userName",new Abstract::STRING(sAccountName)}
        }))
        return false;
    }
    return true;
}

bool Manager_DB::accountChangeAttribs(const std::string &sAccountName, const sAccountAttribs &accountAttribs)
{
    Threads::Sync::Lock_RW lock(mutex);

    if ((accountAttribs.confirmed==false || accountAttribs.enabled==false || accountAttribs.superuser==false) && !isThereAnotherSuperUser(sAccountName))
    {
        return false;
    }

    return sqlConnector->query("UPDATE vauth_v3_accounts SET `enabled`=:enabled,`confirmed`=:confirmed,`superuser`=:superuser WHERE `userName`=:userName;",
                               {
                                   {":enabled",new Abstract::BOOL(accountAttribs.enabled)},
                                   {":confirmed",new Abstract::BOOL(accountAttribs.confirmed)},
                                   {":superuser",new Abstract::BOOL(accountAttribs.superuser)},
                                   {":userName",new Abstract::STRING(sAccountName)}
                               });
}

bool Manager_DB::isAccountDisabled(const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL enabled;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `enabled` FROM vauth_v3_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &enabled });

    if (i->ok && i->query->step())
    {
        return !enabled.getValue();
    }
    return true;
}

bool Manager_DB::isAccountConfirmed(const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL confirmed;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `confirmed` FROM vauth_v3_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &confirmed });

    if (i->ok && i->query->step())
    {
        return confirmed.getValue();
    }
    return false;
}

bool Manager_DB::isAccountSuperUser(const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL superuser;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `superuser` FROM vauth_v3_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &superuser });

    if (i->ok && i->query->step())
    {
        return superuser.getValue();
    }
    return false;
}

std::string Manager_DB::accountGivenName(const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sGivenName;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `givenName` FROM vauth_v3_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &sGivenName });

    if (i->ok && i->query->step())
    {
        return sGivenName.getValue();
    }
    return "";
}

std::string Manager_DB::accountLastName(const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sLastName;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `lastName` FROM vauth_v3_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &sLastName });

    if (i->ok && i->query->step())
    {
        return sLastName.getValue();
    }
    return "";
}

std::string Manager_DB::accountDescription(const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING description;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `description` FROM vauth_v3_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &description });

    if (i->ok && i->query->step())
    {
        return description.getValue();
    }
    return "";
}

std::string Manager_DB::accountEmail(const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING email;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `email` FROM vauth_v3_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &email });

    if (i->ok && i->query->step())
    {
        return email.getValue();
    }
    return "";
}

std::string Manager_DB::accountExtraData(const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING extraData;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `extraData` FROM vauth_v3_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &extraData });

    if (i->ok && i->query->step())
    {
        return extraData.getValue();
    }
    return "";
}

time_t Manager_DB::accountExpirationDate(const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::DATETIME expiration;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `expiration` FROM vauth_v3_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &expiration });
    if (i->ok && i->query->step())
    {
        return expiration.getValue();
    }
    // If can't get this data, the account is expired:
    return 1;
}

void Manager_DB::updateLastLogin(const std::string &sAccountName, const uint32_t &uPassIdx, const sClientDetails &clientDetails)
{
    Threads::Sync::Lock_RW lock(mutex);

    sqlConnector->query("UPDATE vauth_v3_accounts SET `lastLogin`=CURRENT_TIMESTAMP WHERE `userName`=:userName;",
                        {
                            {":userName",new Abstract::STRING(sAccountName)}
                        });

    sqlConnector->query("INSERT INTO vauth_v3_accountlogins(`f_userName`,`f_secretIndex`,`loginDateTime`,`loginIP`,`loginTLSCN`,`loginUserAgent`,`loginExtraData`) "
                        "VALUES "
                        "(:userName,:index,:date,:loginIP,:loginTLSCN,:loginUserAgent,:loginExtraData);",
                        {
                            {":userName",new Abstract::STRING(sAccountName)},
                            {":index",new Abstract::UINT32(uPassIdx)},
                            {":date",new Abstract::DATETIME(time(nullptr))},
                            {":loginIP",new Abstract::STRING(clientDetails.sIPAddr)},
                            {":loginTLSCN",new Abstract::STRING(clientDetails.sTLSCommonName)},
                            {":loginUserAgent",new Abstract::STRING(clientDetails.sUserAgent)},
                            {":loginExtraData",new Abstract::STRING(clientDetails.sExtraData)}
                        }
                        );

}

time_t Manager_DB::accountLastLogin(const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::DATETIME lastLogin;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `lastLogin` FROM vauth_v3_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &lastLogin });
    if (i->ok && i->query->step())
    {
        return lastLogin.getValue();
    }
    // If can't get this data, the account is expired:
    return 1;
}

void Manager_DB::resetBadAttempts(const std::string &sAccountName, const uint32_t &passIndex)
{
    Threads::Sync::Lock_RW lock(mutex);
    sqlConnector->query("UPDATE vauth_v3_accountsecrets SET `badAttempts`='0' WHERE `f_userName`=:userName and `f_secretIndex`=:index;",
                        {
                            {":userName",new Abstract::STRING(sAccountName)},
                            {":index",new Abstract::UINT32(passIndex)}
                        });

}

void Manager_DB::incrementBadAttempts(const std::string &sAccountName, const uint32_t &passIndex)
{
    Threads::Sync::Lock_RW lock(mutex);
    sqlConnector->query("UPDATE vauth_v3_accountsecrets SET `badAttempts`=`badAttempts`+1  WHERE `f_userName`=:userName and `f_secretIndex`=:index;",
                        {
                            {":userName",new Abstract::STRING(sAccountName)},
                            {":index",new Abstract::UINT32(passIndex)}
                        });
}

std::list<sAccountSimpleDetails> Manager_DB::accountsBasicInfoSearch(std::string sSearchWords, uint64_t limit, uint64_t offset)
{
    std::list<sAccountSimpleDetails> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sAccountName,givenName,lastName,email,description;
    Abstract::BOOL superuser,enabled,confirmed;
    Abstract::DATETIME expiration;

    std::string sSqlQuery = "SELECT `userName`,`givenName`,`lastName`,`email`,`description`,`superuser`,`enabled`,`expiration`,`confirmed` FROM vauth_v3_accounts";

    if (!sSearchWords.empty())
    {
        sSearchWords = '%' + sSearchWords + '%';
        sSqlQuery+=" WHERE (`userName` LIKE :SEARCHWORDS OR `givenName` LIKE :SEARCHWORDS OR `lastName` LIKE :SEARCHWORDS OR `email` LIKE :SEARCHWORDS OR `description` LIKE :SEARCHWORDS)";
    }

    if (limit)
        sSqlQuery+=" LIMIT :LIMIT OFFSET :OFFSET";

    sSqlQuery+=";";

    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect(sSqlQuery,
                                          {
                                              {":SEARCHWORDS",new Abstract::STRING(sSearchWords)},
                                              {":LIMIT",new Abstract::UINT64(limit)},
                                              {":OFFSET",new Abstract::UINT64(offset)}
                                          },
                                          { &sAccountName, &givenName, &lastName, &email, &description, &superuser, &enabled, &expiration, &confirmed });
    while (i->ok && i->query->step())
    {
        sAccountSimpleDetails rDetail;

        rDetail.confirmed = confirmed.getValue();
        rDetail.enabled = enabled.getValue();
        rDetail.superuser = superuser.getValue();
        rDetail.sGivenName = givenName.getValue();
        rDetail.sLastName = lastName.getValue();
        rDetail.sDescription = description.getValue();
        rDetail.expired = !expiration.getValue()?false:expiration.getValue()<time(nullptr);
        rDetail.sEmail = email.getValue();
        rDetail.sAccountName = sAccountName.getValue();

        ret.push_back(rDetail);
    }

    return ret;
}

std::set<std::string> Manager_DB::accountsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sAccountName;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `userName` FROM vauth_v3_accounts;",
                                          { },
                                          { &sAccountName });
    while (i->ok && i->query->step())
    {
        ret.insert(sAccountName.getValue());
    }

    return ret;
}

std::set<std::string> Manager_DB::accountGroups(const std::string &sAccountName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING group;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `f_groupName` FROM vauth_v3_groupsaccounts WHERE `f_userName`=:userName;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &group });
    while (i->ok && i->query->step())
    {
        ret.insert(group.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

std::set<sApplicationAttrib> Manager_DB::accountDirectAttribs(const std::string &sAccountName, bool lock)
{
    std::set<sApplicationAttrib> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING appName,attrib;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `f_appName`,`f_attribName` FROM vauth_v3_attribsaccounts WHERE `f_userName`=:userName;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)} },
                                          { &appName,&attrib });
    while (i->ok && i->query->step())
    {
        ret.insert( { appName.getValue(), attrib.getValue() } );
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

bool Manager_DB::superUserAccountExist()
{
    Threads::Sync::Lock_RD lock(mutex);

    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `superuser` FROM vauth_v3_accounts WHERE `superuser`=:superuser LIMIT 1;",
                                          { {":superuser",new Memory::Abstract::BOOL(true)} },
                                          { });

    if (i->ok && i->query->step())
        return true;

    return false;
}


Secret Manager_DB::retrieveSecret(const std::string &sAccountName, uint32_t passIndex, bool *accountFound, bool *indexFound)
{
    Secret ret;
    *indexFound = false;
    *accountFound = false;

    Threads::Sync::Lock_RD lock(mutex);

    Abstract::UINT32 steps,function,badAttempts;
    Abstract::BOOL forcedExpiration;
    Abstract::DATETIME expiration;
    Abstract::STRING salt,hash;

    *accountFound = accountExist(sAccountName);

    if (!*accountFound)
        return ret;

    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `steps`,`forcedExpiration`,`function`,`expiration`,`badAttempts`,`salt`,`hash` FROM vauth_v3_accountsecrets "
                                          "WHERE `f_userName`=:userName AND `f_secretIndex`=:index LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(sAccountName)},
                                            {":index",new Memory::Abstract::UINT32(passIndex)}
                                          },
                                          { &steps, &forcedExpiration, &function, &expiration, &badAttempts, &salt, &hash });

    if (i->ok && i->query->step())
    {
        *indexFound = true;
        ret.gAuthSteps = steps.getValue();
        ret.forceExpiration = forcedExpiration.getValue();
        ret.passwordFunction = (Authentication::Function)function.getValue();
        ret.expiration = expiration.getValue();
        ret.badAttempts = badAttempts.getValue();
        Mantids::Helpers::Encoders::fromHex(salt.getValue(),ret.ssalt,4);
        ret.hash = hash.getValue();
    }
    return ret;
}

bool Manager_DB::isThereAnotherSuperUser(const std::string &sAccountName)
{
    // Check if there is any superuser acount beside this "to be deleted" account...
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `enabled` FROM vauth_v3_accounts WHERE `userName`!=:userName and `superuser`=:superUser and enabled=:enabled LIMIT 1;",
                                          {
                                              {":userName",new Memory::Abstract::STRING(sAccountName)},
                                              {":superUser",new Memory::Abstract::BOOL(true)},
                                              {":enabled",new Memory::Abstract::BOOL(true)}
                                          },
                                          { });

    if (i->ok && i->query->step())
        return true;
    return false;

}

