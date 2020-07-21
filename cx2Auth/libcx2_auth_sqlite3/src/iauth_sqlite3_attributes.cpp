#include "iauth_sqlite3.h"
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Authorization;

bool IAuth_SQLite3::attribAdd(const std::string &attribName, const std::string &attribDescription)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret=_pSQLExecQueryF("INSERT INTO vauth_v2_attribs (name,description) VALUES(?,?);", 2,
                        attribName.c_str(),
                        attribDescription.c_str());
    
    return ret;
}

bool IAuth_SQLite3::attribRemove(const std::string &attribName)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret = _pSQLExecQueryF("DELETE FROM vauth_v2_attribs WHERE name=?;", 1, attribName.c_str());
    
    return ret;
}

bool IAuth_SQLite3::attribGroupAdd(const std::string &attribName, const std::string &groupName)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret=_pSQLExecQueryF("INSERT INTO vauth_v2_attribs_groups (`attrib_name`,`group_name`) VALUES(?,?);", 2,
                        attribName.c_str(),
                        groupName.c_str()
                        );
    
    return ret;
}

bool IAuth_SQLite3::attribGroupRemove(const std::string &attribName, const std::string &groupName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();
    ret = _pSQLExecQueryF("DELETE FROM vauth_v2_attribs_groups WHERE attrib_name=? AND group_name=?;",2, attribName.c_str(), groupName.c_str());
    if (lock) mutex.unlock();
    return ret;
}

bool IAuth_SQLite3::attribAccountAdd(const std::string &attribName, const std::string &accountName)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret=_pSQLExecQueryF("INSERT INTO vauth_v2_attribs_accounts (`attrib_name`,`account_name`) VALUES(?,?);", 2,
                        attribName.c_str(),
                        accountName.c_str()
                        );
    
    return ret;
}

bool IAuth_SQLite3::attribAccountRemove(const std::string &attribName, const std::string &accountName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();
    ret = _pSQLExecQueryF("DELETE FROM vauth_v2_attribs_accounts WHERE attrib_name=? AND account_name=?;", 2, attribName.c_str(), accountName.c_str());
    if (lock) mutex.unlock();
    return ret;
}

bool IAuth_SQLite3::attribChangeDescription(const std::string &attribName, const std::string &attribDescription)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret = _pSQLExecQueryF("UPDATE vauth_v2_attribs SET description=? WHERE name=? ;", 2, attribDescription.c_str(), attribName.c_str());
    
    return ret;
}

std::string IAuth_SQLite3::attribDescription(const std::string &attribName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql = "SELECT description FROM vauth_v2_attribs WHERE name=? LIMIT 1";


    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, attribName.c_str(), attribName.size(), nullptr);

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

std::set<std::string> IAuth_SQLite3::attribsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql= "SELECT name FROM vauth_v2_attribs;";

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

std::set<std::string> IAuth_SQLite3::attribGroups(const std::string &attribName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();
    std::string xsql= "SELECT group_name FROM vauth_v2_attribs_groups WHERE `attrib_name`=?;";

    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, attribName.c_str(), attribName.size(), nullptr);
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

std::set<std::string> IAuth_SQLite3::attribAccounts(const std::string &attribName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();
    std::string xsql= "SELECT account_name FROM vauth_v2_attribs_accounts WHERE `attrib_name`=?;";

    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, attribName.c_str(), attribName.size(), nullptr);
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

bool IAuth_SQLite3::accountValidateDirectAttribute(const std::string &accountName, const std::string &attribName)
{
    bool ret=false;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql= "SELECT account_name FROM vauth_v2_attribs_accounts WHERE `attrib_name`=? AND `account_name`=?;";

    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, attribName.c_str(), attribName.size(), nullptr);
    sqlite3_bind_text(stmt, 2, accountName.c_str(), attribName.size(), nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) ret = true;
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    
    return ret;
}
