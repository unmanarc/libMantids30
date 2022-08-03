#ifndef QUERY_MARIADB_H
#define QUERY_MARIADB_H

#include <mdz_db/query.h>
#include <mysql.h>
#include <vector>

namespace Mantids { namespace Database {

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
    bool exec0(const ExecType & execType, bool recursion);

    bool step0();
    bool postBindInputVars();

private:
    int reconnection(const ExecType &execType, bool recursion);

    bool connectionError();
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
