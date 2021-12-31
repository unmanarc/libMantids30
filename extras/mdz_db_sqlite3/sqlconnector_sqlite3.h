#ifndef SQLCONNECTOR_SQLITE3_H
#define SQLCONNECTOR_SQLITE3_H

#include <mdz_db/sqlconnector.h>
#include "query_sqlite3.h"
#include <sqlite3.h>
#include <mutex>

namespace Mantids { namespace Database {


enum eSqlite3PragmaSyncMode {
    SQLITE3_SYNC_OFF = 0,
    SQLITE3_SYNC_NORMAL = 1,
    SQLITE3_SYNC_FULL = 2,
    SQLITE3_SYNC_EXTRA = 3
};

enum eSqlite3PragmaJournalMode {
    SQLITE3_JOURNAL_OFF,
    SQLITE3_JOURNAL_WAL,
    SQLITE3_JOURNAL_MEMORY,
    SQLITE3_JOURNAL_PERSIST,
    SQLITE3_JOURNAL_TRUNCATE,
    SQLITE3_JOURNAL_DELETE
};

class SQLConnector_SQLite3 : public SQLConnector
{
public:
    SQLConnector_SQLite3();
    ~SQLConnector_SQLite3();

    /**
     * @brief connectInMemory Connect to IN-MEMORY SQLite3 Database (like using connect(":memory:"))
     * reference: https://sqlite.org/inmemorydb.html
     * @return true if connected
     */
    bool connectInMemory();

    /**
     * @brief isOpen Check if the database is open
     * @return true if open.
     */
    bool isOpen();

    /**
     * @brief driverName Get driver Name.
     * @return driver name (SQLITE3)
     */
    std::string driverName() { return "SQLITE3"; }

    /**
     * @brief dbTableExist Check if sqlite3 table exist
     * @param table table name
     * @return true if exist, otherwise false.
     */
    bool dbTableExist(const std::string & table);

    /**
     * @brief prepareQuery Internal function used by the query to prepare the query with the database handler.
     * @param query Query.
     * @return true if succeed.
     */
    void putDatabaseConnectorIntoQuery( Query_SQLite3 * query );

    bool sqlite3PragmaForeignKeys(bool on = true);
    bool sqlite3PragmaJournalMode(const eSqlite3PragmaJournalMode & mode);
    bool sqlite3PragmaSynchronous(const eSqlite3PragmaSyncMode & mode);


    std::string getEscaped(const std::string & value);


protected:
    Query * createQuery0() { return new Query_SQLite3; };
    bool connect0();

private:
    int rc;
    sqlite3 *ppDb;

};
}}

#endif // SQLCONNECTOR_SQLITE3_H
