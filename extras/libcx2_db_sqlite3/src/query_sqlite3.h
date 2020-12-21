#ifndef QUERY_SQLITE3_H
#define QUERY_SQLITE3_H

#include <cx2_db/query.h>
#include <sqlite3.h>

namespace CX2 { namespace Database {

class Query_SQLite3 : public Query
{
public:
    Query_SQLite3();
    ~Query_SQLite3();

    // Direct Query:
    bool exec(const ExecType & execType);
    bool step();

    // Sqlite3 options...

    // Query preparation called from SQL Connector.
    void sqlite3SetDatabaseConnector( sqlite3 *ppDb );
    bool sqlite3IsDone() const;

private:
    sqlite3_stmt *stmt;
    sqlite3 *ppDb;
};
}}

#endif // QUERY_SQLITE3_H
