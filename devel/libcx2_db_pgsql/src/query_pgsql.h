#ifndef QUERY_PGSQL_H
#define QUERY_PGSQL_H

#include <cx2_db/query.h>

namespace CX2 { namespace Database {

class Query_PostgreSQL : public Query
{
public:
    Query_PostgreSQL();
    bool exec(const ExecType & execType);
    bool step();

};
}}

#endif // QUERY_PGSQL_H
