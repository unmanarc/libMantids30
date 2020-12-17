#ifndef SQLCONNECTOR_PGSQL_H
#define SQLCONNECTOR_PGSQL_H

#include <cx2_db/sqlconnector.h>
#include "query_pgsql.h"
#include <libpq-fe.h>

namespace CX2 { namespace Database {

class SQLConnector_PostgreSQL : public SQLConnector
{
public:
    SQLConnector_PostgreSQL();
    ~SQLConnector_PostgreSQL();
    std::string driverName() { return "PGSQL"; }
protected:
    Query * createQuery0() { return new Query_PostgreSQL; };
    bool connect0();
private:
    PGconn     *conn;

};
}}

#endif // SQLCONNECTOR_PGSQL_H
