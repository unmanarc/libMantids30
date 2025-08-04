#include "sqlconnector_sqlite3.h"
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
    std::shared_ptr<SQLConnector::QueryInstance> i = qSelect("select sql from sqlite_master where tbl_name=:tbl;",
                                                             {{":tbl",std::make_shared<Memory::Abstract::STRING>(table)}},
                                                               {}
                                                               );

    if (i->getResultsOK())
        return i->query->step();
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
    if (m_ppDb)
    {
        sqlite3_close(m_ppDb);
        m_ppDb = nullptr;
    }

    m_rc = sqlite3_open(m_dbFilePath.c_str(),&m_ppDb);
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
    return query(attachQuery);
}

bool SQLConnector_SQLite3::detach0(const std::string &schemeName)
{
    std::string detachQuery = "DETACH DATABASE " + schemeName + ";";
    return query(detachQuery);
}
