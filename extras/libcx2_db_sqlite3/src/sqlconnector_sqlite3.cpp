#include "sqlconnector_sqlite3.h"

using namespace CX2::Database;

SQLConnector_SQLite3::SQLConnector_SQLite3()
{
    ppDb = nullptr;
}

SQLConnector_SQLite3::~SQLConnector_SQLite3()
{
    if (ppDb)
        sqlite3_close(ppDb);
}

bool SQLConnector_SQLite3::dbTableExist(const std::string &table)
{
    std::unique_lock<std::mutex> lock(mtDatabaseLock);

    bool ret;
    std::string xsql = "select sql from sqlite_master where tbl_name=?;";
    sqlite3_stmt * stmt = nullptr;
    sqlite3_prepare_v2(ppDb, xsql.c_str(), xsql.size() + 1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, table.c_str(), table.size(), nullptr);
    int s = sqlite3_step(stmt);
    ret = (s == SQLITE_ROW ? true : false);
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);

    return ret;
}

bool SQLConnector_SQLite3::prepareQuery(Query_SQLite3 *query)
{
    std::unique_lock<std::mutex> lock(mtDatabaseLock);

    return query->sqlite3Prepare(ppDb);
}

bool SQLConnector_SQLite3::connect0()
{
    rc = sqlite3_open(dbFilePath.c_str(), &ppDb);
    if (rc)
    {
        lastSQLError = "Error openning the database file";
        return false;
    }
    return true;
}
