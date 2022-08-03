#include "sqlconnector_sqlite3.h"
#include <mdz_mem_vars/a_string.h>
#include <unistd.h>

using namespace Mantids::Database;

SQLConnector_SQLite3::SQLConnector_SQLite3()
{
    ppDb = nullptr;
}

SQLConnector_SQLite3::~SQLConnector_SQLite3()
{
    if (ppDb)
        sqlite3_close(ppDb);
}

bool SQLConnector_SQLite3::connectInMemory()
{
    return connect(":memory:");
}

bool SQLConnector_SQLite3::isOpen()
{
    return ppDb != nullptr && !rc;
}

bool SQLConnector_SQLite3::dbTableExist(const std::string &table)
{
    // Select Query:
    std::shared_ptr<QueryInstance> i = qSelect("select sql from sqlite_master where tbl_name=:tbl;",
                                               {{":tbl",new Memory::Abstract::STRING(table)}},
                                               {}
                                               );

    if (i->ok)
        return i->query->step();
    else
        return false;
}

void SQLConnector_SQLite3::putDatabaseConnectorIntoQuery(Query_SQLite3 *query)
{
    query->sqlite3SetDatabaseConnector(ppDb);
}

bool SQLConnector_SQLite3::sqlite3PragmaForeignKeys(bool on)
{
    if (on)
        return query("PRAGMA foreign_keys = ON;");
    else
        return query("PRAGMA foreign_keys = OFF;");
}

bool SQLConnector_SQLite3::sqlite3PragmaJournalMode(const eSqlite3PragmaJournalMode &mode)
{
    switch (mode) {
    case SQLITE3_JOURNAL_OFF:
        return query("PRAGMA journal_mode = OFF;");
    case SQLITE3_JOURNAL_WAL:
        return query("PRAGMA journal_mode = WAL;");
    case SQLITE3_JOURNAL_MEMORY:
        return query("PRAGMA journal_mode = MEMORY;");
    case SQLITE3_JOURNAL_PERSIST:
        return query("PRAGMA journal_mode = PERSIST;");
    case SQLITE3_JOURNAL_TRUNCATE:
        return query("PRAGMA journal_mode = TRUNCATE;");
    case SQLITE3_JOURNAL_DELETE:
        return query("PRAGMA journal_mode = DELETE;");
    }
    return false;
}

bool SQLConnector_SQLite3::sqlite3PragmaSynchronous(const eSqlite3PragmaSyncMode &mode)
{
    switch (mode) {
    case SQLITE3_SYNC_OFF:
        return query("PRAGMA synchronous = OFF;");
    case SQLITE3_SYNC_NORMAL:
        return query("PRAGMA synchronous = NORMAL;");
    case SQLITE3_SYNC_FULL:
        return query("PRAGMA synchronous = FULL;");
    case SQLITE3_SYNC_EXTRA:
        return query("PRAGMA synchronous = EXTRA;");
    }
    return false;
}

std::string SQLConnector_SQLite3::getEscaped(const std::string &v)
{
    char * cEscaped = sqlite3_mprintf("%Q", v.c_str());
    std::string sEscaped = cEscaped;
    sqlite3_free(cEscaped);
    return sEscaped;
}

bool SQLConnector_SQLite3::connect0()
{
    if (ppDb)
    {
        sqlite3_close(ppDb);
        ppDb = nullptr;
    }

    rc = sqlite3_open(dbFilePath.c_str(), &ppDb);
    if (rc)
    {
        lastSQLError = "Error openning the database file";
        return false;
    }
    sqlite3PragmaForeignKeys();
    return true;
}
