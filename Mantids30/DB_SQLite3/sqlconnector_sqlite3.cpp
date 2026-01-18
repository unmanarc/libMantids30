#include "sqlconnector_sqlite3.h"
#include "Mantids30/DB/sqlconnector.h"
#include <Mantids30/Memory/a_string.h>
#include <unistd.h>

using namespace Mantids30::Database;

SQLConnector_SQLite3::SQLConnector_SQLite3()
{
    m_ppDb = nullptr;
}

SQLConnector_SQLite3::~SQLConnector_SQLite3()
{
    if (m_ppDb)
        sqlite3_close(m_ppDb);
}

bool SQLConnector_SQLite3::connectInMemory()
{
    return connect(":memory:");
}

bool SQLConnector_SQLite3::isOpen()
{
    return m_ppDb != nullptr && !m_rc;
}

bool SQLConnector_SQLite3::dbTableExist(const std::string &table)
{
    // Select Query:
    auto i = qSelect("select sql from sqlite_master where tbl_name=:tbl;", {{":tbl", std::make_shared<Memory::Abstract::STRING>(table)}}, {});

    if (i && i->isSuccessful())
        return i->step();
    else
        return false;
}

void SQLConnector_SQLite3::putDatabaseConnectorIntoQuery(Query_SQLite3 *query)
{
    query->setDatabaseConnectionHandler(m_ppDb);
}

bool SQLConnector_SQLite3::sqlite3PragmaForeignKeys(bool on)
{
    if (on)
        return qExecuteEx("PRAGMA foreign_keys = ON;");
    else
        return qExecuteEx("PRAGMA foreign_keys = OFF;");
}

bool SQLConnector_SQLite3::sqlite3PragmaJournalMode(const eSqlite3PragmaJournalMode &mode)
{
    switch (mode)
    {
    case SQLITE3_JOURNAL_OFF:
        return qExecuteEx("PRAGMA journal_mode = OFF;");
    case SQLITE3_JOURNAL_WAL:
        return qExecuteEx("PRAGMA journal_mode = WAL;");
    case SQLITE3_JOURNAL_MEMORY:
        return qExecuteEx("PRAGMA journal_mode = MEMORY;");
    case SQLITE3_JOURNAL_PERSIST:
        return qExecuteEx("PRAGMA journal_mode = PERSIST;");
    case SQLITE3_JOURNAL_TRUNCATE:
        return qExecuteEx("PRAGMA journal_mode = TRUNCATE;");
    case SQLITE3_JOURNAL_DELETE:
        return qExecuteEx("PRAGMA journal_mode = DELETE;");
    }
    return false;
}

bool SQLConnector_SQLite3::sqlite3PragmaSynchronous(const eSqlite3PragmaSyncMode &mode)
{
    switch (mode)
    {
    case SQLITE3_SYNC_OFF:
        return qExecuteEx("PRAGMA synchronous = OFF;");
    case SQLITE3_SYNC_NORMAL:
        return qExecuteEx("PRAGMA synchronous = NORMAL;");
    case SQLITE3_SYNC_FULL:
        return qExecuteEx("PRAGMA synchronous = FULL;");
    case SQLITE3_SYNC_EXTRA:
        return qExecuteEx("PRAGMA synchronous = EXTRA;");
    }
    return false;
}

std::string SQLConnector_SQLite3::getEscaped(const std::string &v)
{
    char *cEscaped = sqlite3_mprintf("%Q", v.c_str());
    std::string sEscaped = cEscaped;
    sqlite3_free(cEscaped);
    return sEscaped;
}

bool SQLConnector_SQLite3::beginTransaction()
{
    return qExecuteEx("BEGIN TRANSACTION;");
}

bool SQLConnector_SQLite3::rollbackTransaction()
{
    return qExecuteEx("ROLLBACK;");
}

bool SQLConnector_SQLite3::commitTransaction()
{
    return qExecuteEx("COMMIT TRANSACTION");
}

bool SQLConnector_SQLite3::connect0()
{
    if (m_ppDb)
    {
        sqlite3_close(m_ppDb);
        m_ppDb = nullptr;
    }

    m_rc = sqlite3_open(m_dbFilePath.c_str(), &m_ppDb);
    if (m_rc)
    {
        m_lastSQLError = "Error openning the database file";
        return false;
    }
    sqlite3PragmaForeignKeys();
    return true;
}

bool SQLConnector_SQLite3::attach0(const std::string &dbFilePath, const std::string &schemeName)
{
    std::string attachQuery = "ATTACH DATABASE '" + dbFilePath + "' AS " + schemeName + ";";
    return qExecuteEx(attachQuery);
}

bool SQLConnector_SQLite3::detach0(const std::string &schemeName)
{
    std::string detachQuery = "DETACH DATABASE " + schemeName + ";";
    return qExecuteEx(detachQuery);
}
