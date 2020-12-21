#ifndef QUERY_MARIADB_H
#define QUERY_MARIADB_H

#include <cx2_db/query.h>
#include <mysql/mysql.h>
#include <vector>

namespace CX2 { namespace Database {

class Query_MariaDB : public Query
{
public:
    Query_MariaDB();
    ~Query_MariaDB();

    // Direct Query:
    bool exec(const ExecType & execType);
    bool step();

    // MariaDB specific functions:
    void mariaDBSetDatabaseConnector( MYSQL *dbCnt );

    my_ulonglong getLastInsertRowID() const;

protected:
    bool postBindInputVars();
    bool postBindResultVars();

private:
    MYSQL * dbCnt;

    MYSQL_STMT * stmt;
    MYSQL_BIND * bindedInputParams;
    MYSQL_BIND * bindedResultsParams;

    my_bool * bIsNull;

    bool bDeallocateResultSet;
    bool bFetchLastInsertRowID;

    std::vector<std::string> keysByPos;

};
}}

#endif // QUERY_MARIADB_H
