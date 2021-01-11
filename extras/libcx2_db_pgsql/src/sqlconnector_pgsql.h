#ifndef SQLCONNECTOR_PGSQL_H
#define SQLCONNECTOR_PGSQL_H

#include <cx2_db/sqlconnector.h>
#include "query_pgsql.h"

#if __has_include(<libpq-fe.h>)
# include "<libpq-fe.h>"
#elif __has_include(<postgresql/libpq-fe.h>)
# include <postgresql/libpq-fe.h>
#endif

namespace CX2 { namespace Database {

class SQLConnector_PostgreSQL : public SQLConnector
{
public:
    SQLConnector_PostgreSQL();
    ~SQLConnector_PostgreSQL();
    std::string driverName() { return "PGSQL"; }

    // Query:

    /**
     * @brief prepareQuery Internal function used by the query to prepare the query with the database handler.
     * @param query Query.
     * @return true if succeed.
     */
    void getDatabaseConnector( Query_PostgreSQL * query );

    /**
     * @brief dbTableExist Check if postgresql table exist
     * @param table table name
     * @return true if exist, otherwise false.
     */
    bool dbTableExist(const std::string & table);

    // Escape:
    std::string getEscaped(const std::string &v);
    int getPsqlEscapeError() const;

    // To be used before the connection establishment:

    void psqlSetConnectionTimeout(const uint32_t & value);
    void psqlSetConnectionOptions(const std::string &value);
    void psqlSetConnectionSSLMode(const std::string &value);

protected:
    Query * createQuery0() { return new Query_PostgreSQL; };
    bool connect0();
private:
    void fillConnectionArray();

    char ** getConnectionKeys();
    char ** getConnectionValues();

    void destroyArray(char ** values);

    PGconn * conn;

    int psqlEscapeError;
    std::map<std::string,std::string> connectionValues;

    uint32_t uConnectionTimeout;
    std::string sConnectionOptions, sConnectionSSLMode;
};
}}

#endif // SQLCONNECTOR_PGSQL_H
