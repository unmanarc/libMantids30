#include "manager_sqlite3.h"

#include <limits>
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Authentication;

bool Manager_SQLite3::accountAdd(const std::string &accountName, const Secret &passData, const std::string &email, const std::string &accountDescription, const std::string &extraData, time_t expirationDate, bool enabled, bool confirmed, bool superuser)
{
    bool ret = false;

    Threads::Sync::Lock_RW lock(mutex);
    ret=_pSQLExecQueryF("INSERT INTO vauth_v2_accounts (name,email,description,extraData,superuser) VALUES(?,?,?,?,?);", 5,
                        accountName.c_str(),
                        email.c_str(),
                        accountDescription.c_str(),
                        extraData.c_str(),
                        std::to_string(superuser?1:0).c_str());
    if (ret)
    {
        ret=_pSQLExecQueryF("INSERT INTO vauth_v2_account_activation (`account`,`enabled`,`expiration`,`confirmed`,`confirmationToken`) VALUES(?,?,?,?,?);", 5,
                            accountName.c_str(),
                            std::to_string(enabled?1:0).c_str(),
                            std::to_string(expirationDate).c_str(),
                            std::to_string(confirmed?1:0).c_str(),
                            genRandomConfirmationToken().c_str()
                            );
    }
    if (ret)
    {
        ret=_pSQLExecQueryF("INSERT INTO vauth_v2_account_passwords (`index`,`account`,`hash`,`expiration`,`mode`,`salt`,`forcedExpiration`,`steps`) VALUES(0,?,?,?,?,?,?,?,?);", 8,
                            accountName.c_str(),
                            passData.hash.c_str(),
                            std::to_string(passData.expiration).c_str(),
                            std::to_string(passData.passwordFunction).c_str(),
                            CX2::Helpers::Encoders::toHex(passData.ssalt,4).c_str(),
                            std::to_string(passData.forceExpiration?1:0).c_str(),
                            std::to_string(passData.gAuthSteps).c_str()
                            );
    }


    return ret;
}

std::string Manager_SQLite3::accountConfirmationToken(const std::string &accountName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql = "SELECT confirmationToken FROM vauth_v2_account_activation WHERE account=? LIMIT 1";

    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, accountName.c_str(), accountName.size(), nullptr);

    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
        ret = (const char *)sqlite3_column_text(stmt, 0);
    else
        //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);

    return ret;
}

bool Manager_SQLite3::accountRemove(const std::string &accountName)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret = _pSQLExecQueryF("DELETE FROM vauth_v2_accounts WHERE name=?;", 1, accountName.c_str());

    return ret;
}

bool Manager_SQLite3::accountDisable(const std::string &accountName, bool disabled)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret = _pSQLExecQueryF("UPDATE vauth_v2_account_activation SET enabled=? WHERE name=?;", 2, disabled?"0":"1", accountName.c_str());

    return ret;
}

bool Manager_SQLite3::accountConfirm(const std::string &accountName, const std::string &confirmationToken)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);

    std::string xsql = "SELECT confirmationToken FROM vauth_v2_account_activation WHERE account=? LIMIT 1";
    std::string dbConfirmationToken;

    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, accountName.c_str(), accountName.size(), nullptr);

    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
        dbConfirmationToken = (const char *)sqlite3_column_text(stmt, 0);
    else
        //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);

    if (!dbConfirmationToken.empty() && confirmationToken==dbConfirmationToken)
    {
        ret = _pSQLExecQueryF("UPDATE vauth_v2_account_activation SET confirmed=? WHERE account=?;", 2, "1", accountName.c_str());
    }


    return ret;
}

bool Manager_SQLite3::accountChangeSecret(const std::string &accountName, const Secret &passwordData, uint32_t passIndex)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    _pSQLExecQueryF("DELETE FROM vauth_v2_account_passwords WHERE account=? and `index`=?",2, accountName.c_str(), std::to_string(passIndex).c_str());
    ret=_pSQLExecQueryF("INSERT INTO vauth_v2_account_passwords (`index`,`account`,`hash`,`expiration`,`mode`,`salt`,`forcedExpiration`,`steps`) VALUES(?,?,?,?,?,?,?,?);", 8,
                        std::to_string(passIndex).c_str(),
                        accountName.c_str(),
                        passwordData.hash.c_str(),
                        std::to_string(passwordData.expiration).c_str(),
                        std::to_string(passwordData.passwordFunction).c_str(),
                        CX2::Helpers::Encoders::toHex(passwordData.ssalt,4).c_str(),
                        std::to_string(passwordData.forceExpiration?1:0).c_str(),
                        std::to_string(passwordData.gAuthSteps).c_str()
                        );

    return ret;
}

bool Manager_SQLite3::accountChangeDescription(const std::string &accountName, const std::string &description)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret = _pSQLExecQueryF("UPDATE vauth_v2_accounts SET description=? WHERE name=? ;", 2, description.c_str(), accountName.c_str());

    return ret;
}

bool Manager_SQLite3::accountChangeEmail(const std::string &accountName, const std::string &email)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret = _pSQLExecQueryF("UPDATE vauth_v2_accounts SET email=? WHERE name=? ;", 2, email.c_str(), accountName.c_str());

    return ret;
}

bool Manager_SQLite3::accountChangeExtraData(const std::string &accountName, const std::string &extraData)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret = _pSQLExecQueryF("UPDATE vauth_v2_accounts SET extraData=? WHERE name=? ;", 2, extraData.c_str(), accountName.c_str());

    return ret;
}

bool Manager_SQLite3::accountChangeExpiration(const std::string &accountName, time_t expiration)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret = _pSQLExecQueryF("UPDATE vauth_v2_account_activation SET expiration=? WHERE account=? ;", 2, std::to_string(expiration).c_str(), accountName.c_str());

    return ret;
}

bool Manager_SQLite3::isAccountDisabled(const std::string &accountName)
{
    bool ret = true;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql = "SELECT enabled FROM vauth_v2_account_activation WHERE account=? LIMIT 1";


    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, accountName.c_str(), accountName.size(), nullptr);


    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
        ret = sqlite3_column_int(stmt, 0)==0?true:false;
    else
        //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);

    return ret;
}

bool Manager_SQLite3::isAccountConfirmed(const std::string &accountName)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql = "SELECT confirmed FROM vauth_v2_account_activation WHERE account=? LIMIT 1";


    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, accountName.c_str(), accountName.size(), nullptr);


    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
        ret = sqlite3_column_int(stmt, 0)==1?true:false;
    else
        //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);

    return ret;
}

bool Manager_SQLite3::isAccountSuperUser(const std::string &accountName)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql = "SELECT superuser FROM vauth_v2_accounts WHERE name=? LIMIT 1";


    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, accountName.c_str(), accountName.size(), nullptr);


    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
        ret = sqlite3_column_int(stmt, 0)==1?true:false;
    else
        //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);

    return ret;
}

std::string Manager_SQLite3::accountDescription(const std::string &accountName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql = "SELECT description FROM vauth_v2_accounts WHERE name=? LIMIT 1";


    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, accountName.c_str(), accountName.size(), nullptr);


    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
        ret = (const char *)sqlite3_column_text(stmt, 0);
    else
        //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);

    return ret;
}

std::string Manager_SQLite3::accountEmail(const std::string &accountName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql = "SELECT email FROM vauth_v2_accounts WHERE name=? LIMIT 1";


    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, accountName.c_str(), accountName.size(), nullptr);


    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
        ret = (const char *)sqlite3_column_text(stmt, 0);
    else
        //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);

    return ret;
}

std::string Manager_SQLite3::accountExtraData(const std::string &accountName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql = "SELECT extraData FROM vauth_v2_accounts WHERE name=? LIMIT 1";


    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, accountName.c_str(), accountName.size(), nullptr);


    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
        ret = (const char *)sqlite3_column_text(stmt, 0);
    else
        //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);

    return ret;
}

time_t Manager_SQLite3::accountExpirationDate(const std::string &accountName)
{
    time_t ret = std::numeric_limits<time_t>::max();
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql = "SELECT expiration FROM vauth_v2_account_activation WHERE account=? LIMIT 1";


    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, accountName.c_str(), accountName.size(), nullptr);


    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
        ret = (time_t) sqlite3_column_int(stmt, 0);
    else
        //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);

    return ret;
}

std::set<std::string> Manager_SQLite3::accountsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql= "SELECT name FROM vauth_v2_accounts;";

    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    for (;;)
    {
        int s = sqlite3_step(stmt);
        if (s == SQLITE_ROW)
        {
            ret.insert((const char *)sqlite3_column_text(stmt, 0));
        }
        else if (s == SQLITE_DONE)
        {
            break;
        }
        else
        {
            //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));
            break;
        }
    }
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);


    return ret;
}

std::set<std::string> Manager_SQLite3::accountGroups(const std::string &accountName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    std::string xsql= "SELECT group_name FROM vauth_v2_groups_accounts WHERE  account_name=?;";

    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, accountName.c_str(), accountName.size(), nullptr);

    for (;;)
    {
        int s = sqlite3_step(stmt);
        if (s == SQLITE_ROW)
        {
            ret.insert((const char *)sqlite3_column_text(stmt, 0));
        }
        else if (s == SQLITE_DONE)
        {
            break;
        }
        else
        {
            //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));
            break;
        }
    }
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    if (lock) mutex.unlock_shared();
    return ret;
}

std::set<std::string> Manager_SQLite3::accountDirectAttribs(const std::string &accountName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    std::string xsql= "SELECT attrib_name FROM vauth_v2_attribs_accounts WHERE account_name=?;";

    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, accountName.c_str(), accountName.size(), nullptr);

    for (;;)
    {
        int s = sqlite3_step(stmt);
        if (s == SQLITE_ROW)
        {
            ret.insert((const char *)sqlite3_column_text(stmt, 0));
        }
        else if (s == SQLITE_DONE)
        {
            break;
        }
        else
        {
            //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));
            break;
        }
    }
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    if (lock) mutex.unlock_shared();
    return ret;
}

Secret Manager_SQLite3::retrieveSecret(const std::string &accountName, uint32_t passIndex, bool *found)
{
    Secret ret;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql = "SELECT steps,forcedExpiration,mode,expiration,salt,hash FROM vauth_v2_account_passwords WHERE `account`=? AND `index`=? LIMIT 1";

    *found = false;

    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));
    sqlite3_bind_text(stmt, 1, accountName.c_str(), accountName.size(), nullptr);

    sqlite3_bind_int(stmt, 3, (int)passIndex);

    int s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
    {
        *found = true;
        ret.gAuthSteps = (uint32_t)sqlite3_column_int(stmt, 0);
        ret.forceExpiration = sqlite3_column_int(stmt, 1)==0?false:true;
        ret.passwordFunction = (Authentication::Function)sqlite3_column_int(stmt, 2);
        ret.expiration = (time_t)sqlite3_column_int64(stmt, 3);
        std::string salt = (const char *)sqlite3_column_text(stmt, 4);
        CX2::Helpers::Encoders::fromHex(salt,ret.ssalt,4);
        ret.hash = (const char *)sqlite3_column_text(stmt, 5);
    }
    else
        //fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(ppDb));

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);

    return ret;
}
