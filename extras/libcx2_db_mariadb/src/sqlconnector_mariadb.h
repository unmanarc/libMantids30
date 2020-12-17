#ifndef SQLCONNECTOR_MARIADB_H
#define SQLCONNECTOR_MARIADB_H

#include <mysql/mysql.h>
#include <cx2_db/sqlconnector.h>
#include "query_mariadb.h"

namespace CX2 { namespace Database {

class SQLConnector_MariaDB : public SQLConnector
{
public:
    SQLConnector_MariaDB();
    ~SQLConnector_MariaDB();

    std::string driverName() { return "MARIADB"; }


    /**
     * @brief prepareQuery Internal function used by the query to prepare the query with the database handler.
     * @param query Query.
     * @return true if succeed.
     */
    bool prepareQuery( Query_MariaDB * query );



protected:
    Query * createQuery0() { return new Query_MariaDB; };
    bool connect0();
private:
    MYSQL *dbCnt;
    std::mutex mtDatabaseLock;

};
}}

#endif // SQLCONNECTOR_MARIADB_H
