#ifndef QUERY_PGSQL_H
#define QUERY_PGSQL_H

#include <cx2_db/query.h>

#if __has_include(<libpq-fe.h>)
# include <libpq-fe.h>
#elif __has_include(<postgresql/libpq-fe.h>)
# include <postgresql/libpq-fe.h>
#endif

namespace CX2 { namespace Database {

class Query_PostgreSQL : public Query
{
public:
    Query_PostgreSQL();
    ~Query_PostgreSQL();
    bool exec(const ExecType & execType);


    // PostgreSQL specific functions:
    void psqlSetDatabaseConnector(PGconn *conn );
    ExecStatusType psqlGetExecStatus() const;

protected:
    bool step0();
    bool postBindInputVars();
private:
    std::vector<std::string> keysByPos;

    size_t paramCount;
    char ** paramValues;
    int * paramLengths;
    int * paramFormats;

    ExecStatusType execStatus;

    PGconn *dbCnt;
    PGresult* result;
    int currentRow;
};
}}

#endif // QUERY_PGSQL_H
