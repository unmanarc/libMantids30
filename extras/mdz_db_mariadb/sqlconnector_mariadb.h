#ifndef SQLCONNECTOR_MARIADB_H
#define SQLCONNECTOR_MARIADB_H

#include <mysql.h>
#include <mdz_db/sqlconnector.h>
#include "query_mariadb.h"

namespace Mantids { namespace Database {

class SQLConnector_MariaDB : public SQLConnector
{
public:
    SQLConnector_MariaDB();
    ~SQLConnector_MariaDB();

    std::string driverName() { return "MARIADB"; }


    bool isOpen();

    /**
     * @brief prepareQuery Internal function used by the query to prepare the query with the database handler.
     * @param query Query.
     * @return true if succeed.
     */
    void getDatabaseConnector( Query_MariaDB * query );


    /**
     * @brief dbTableExist Check if mariadb table exist
     * @param table table name
     * @return true if exist, otherwise false.
     */
    bool dbTableExist(const std::string & table);
    std::string getEscaped(const std::string &v);

protected:
    Query * createQuery0() { return new Query_MariaDB; };
    bool connect0();
private:
    MYSQL *dbCnt;

};
}}

#endif // SQLCONNECTOR_MARIADB_H
