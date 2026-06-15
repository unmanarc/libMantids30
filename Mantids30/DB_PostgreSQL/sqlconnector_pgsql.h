#pragma once

#include "query_pgsql.h"
#include <Mantids30/DB/sqlconnector.h>

//#if __has_include(<libpq-fe.h>)
#include <libpq-fe.h>
//#elif __has_include(<postgresql/libpq-fe.h>)
//# include <postgresql/libpq-fe.h>
//#endif

namespace Mantids30::Database {

class SQLConnector_PostgreSQL : public SQLConnector
{
public:
    SQLConnector_PostgreSQL();
    ~SQLConnector_PostgreSQL() override;
    std::string driverName() override { return "PGSQL"; }

    bool isOpen() override;
    // Query:

    /**
     * @brief prepareQuery Internal function used by the query to prepare the query with the database handler.
     * @param query Query.
     * @return true if succeed.
     */
    void getDatabaseConnector(Query_PostgreSQL *query);

    /**
     * @brief dbTableExist Check if postgresql table exist
     * @param table table name
     * @return true if exist, otherwise false.
     */
    [[nodiscard]] bool dbTableExist(const std::string &table) override;

    // Escape:
    [[nodiscard]] std::string getEscaped(const std::string &v) override;
    [[nodiscard]] int getPsqlEscapeError() const;

    // To be used before the connection establishment:

    void psqlSetConnectionTimeout(const uint32_t &value);
    void psqlSetConnectionOptions(const std::string &value);
    void psqlSetConnectionSSLMode(const std::string &value);

protected:
    std::shared_ptr<Query> createQuery0() override { return std::make_shared<Query_PostgreSQL>(); };
    bool connect0() override;

private:
    void fillConnectionArray();

    char **getConnectionKeys();
    char **getConnectionValues();

    void destroyArray(char **values);

    PGconn *m_databaseConnectionHandler;

    int m_psqlEscapeError;
    std::map<std::string, std::string> m_connectionValues;

    uint32_t m_connectionTimeout;
    std::string m_connectionOptions, m_connectionSSLMode;
};
} // namespace Mantids30::Database
