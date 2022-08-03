#ifndef QUERY_SQLITE3_H
#define QUERY_SQLITE3_H

#include <mdz_db/query.h>
#include <sqlite3.h>

namespace Mantids { namespace Database {

class Query_SQLite3 : public Query
{
public:
    Query_SQLite3();
    ~Query_SQLite3();


    // Sqlite3 options...

    // Query preparation called from SQL Connector.
    void sqlite3SetDatabaseConnector( sqlite3 *ppDb );
    bool sqlite3IsDone() const;
protected:
    // Direct Query:
    bool exec0(const ExecType & execType, bool recursion);
    bool step0();

private:
    sqlite3_stmt *stmt;
    sqlite3 *ppDb;
};
}}

#endif // QUERY_SQLITE3_H
