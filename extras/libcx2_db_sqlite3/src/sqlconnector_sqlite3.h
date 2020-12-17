#ifndef SQLCONNECTOR_SQLITE3_H
#define SQLCONNECTOR_SQLITE3_H

#include <cx2_db/sqlconnector.h>
#include "query_sqlite3.h"
#include <sqlite3.h>
#include <mutex>

namespace CX2 { namespace Database {

class SQLConnector_SQLite3 : public SQLConnector
{
public:
    SQLConnector_SQLite3();
    ~SQLConnector_SQLite3();

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
    bool prepareQuery( Query_SQLite3 * query );



protected:
    Query * createQuery0() { return new Query_SQLite3; };
    bool connect0();

private:
    int rc;
    sqlite3 *ppDb;
    std::mutex mtDatabaseLock;

};
}}

#endif // SQLCONNECTOR_SQLITE3_H
