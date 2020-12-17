#include "manager_sqlite3.h"
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Authentication;

bool Manager_SQLite3::groupAdd(const std::string &groupName, const std::string &groupDescription)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret=_pSQLExecQueryF("INSERT INTO vauth_v2_groups (name,description) VALUES(?,?);", 2,
                        groupName.c_str(),
                        groupDescription.c_str());
    
    return ret;
}

bool Manager_SQLite3::groupRemove(const std::string &groupName)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret = _pSQLExecQueryF("DELETE FROM vauth_v2_groups WHERE name=?;", 1, groupName.c_str());
    return ret;
}

bool Manager_SQLite3::groupExist(const std::string &groupName)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql= "SELECT name FROM vauth_v2_groups WHERE `name`=?;";
    
    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, groupName.c_str(), groupName.size(), nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) ret = true;
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    
    return ret;
}

bool Manager_SQLite3::groupAccountAdd(const std::string &groupName, const std::string &accountName)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);

    ret=_pSQLExecQueryF("INSERT INTO vauth_v2_groups_accounts (`group_name`,`account_name`) VALUES(?,?);", 2,
                        groupName.c_str(),
                        accountName.c_str()
                        );

    
    return ret;
}

bool Manager_SQLite3::groupAccountRemove(const std::string &groupName, const std::string &accountName, bool lock)
{
    bool ret = false;

    if (lock) mutex.lock();
    ret = _pSQLExecQueryF("DELETE FROM vauth_v2_groups_accounts WHERE group_name=? AND account_name=?;", 2, groupName.c_str(), accountName.c_str());
    if (lock) mutex.unlock();

    return ret;
}

bool Manager_SQLite3::groupChangeDescription(const std::string &groupName, const std::string &groupDescription)
{
    bool ret = false;
    Threads::Sync::Lock_RW lock(mutex);
    ret = _pSQLExecQueryF("UPDATE vauth_v2_groups SET description=? WHERE name=? ;", 2, groupDescription.c_str(), groupName.c_str());
    
    return ret;
}

bool Manager_SQLite3::groupValidateAttribute(const std::string &groupName, const std::string &attribName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock_shared();
    std::string xsql= "SELECT group_name FROM vauth_v2_attribs_groups WHERE `attrib_name`=? AND `group_name`=?;";
    
    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, attribName.c_str(), attribName.size(), nullptr);
    sqlite3_bind_text(stmt, 2, groupName.c_str(), attribName.size(), nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) ret = true;
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    if (lock) mutex.unlock_shared();
    return ret;
}

std::string Manager_SQLite3::groupDescription(const std::string &groupName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);
    std::string xsql = "SELECT description FROM vauth_v2_groups WHERE name=? LIMIT 1";
    

    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, groupName.c_str(), groupName.size(), nullptr);
    

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

std::set<std::string> Manager_SQLite3::groupsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    std::string xsql= "SELECT name FROM vauth_v2_groups;";
    
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

std::set<std::string> Manager_SQLite3::groupAttribs(const std::string &groupName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    std::string xsql= "SELECT attrib_name FROM vauth_v2_attribs_groups WHERE group_name=?;";
    
    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, groupName.c_str(), groupName.size(), nullptr);
    
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

std::set<std::string> Manager_SQLite3::groupAccounts(const std::string &groupName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    std::string xsql= "SELECT account_name FROM vauth_v2_groups_accounts WHERE group_name=?;";
    
    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, groupName.c_str(), groupName.size(), nullptr);
    
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

