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

    // MariaDB specific functions:
    void mariaDBSetDatabaseConnector( MYSQL *dbCnt );

    my_ulonglong getLastInsertRowID() const;

protected:
    bool step0();
    bool postBindInputVars();

private:
    unsigned long mariaDBfetchVarSize(const size_t & col , const enum_field_types &fieldType = MYSQL_TYPE_STRING);

    MYSQL * dbCnt;

    MYSQL_STMT * stmt;
    MYSQL_BIND * bindedInputParams;
    MYSQL_BIND * bindedResultsParams;

    //bool * bIsNull;

    std::vector<unsigned long> bindedResultVarSizes;

    bool bFetchLastInsertRowID;

    std::vector<std::string> keysByPos;

};
}}

#endif // QUERY_MARIADB_H
