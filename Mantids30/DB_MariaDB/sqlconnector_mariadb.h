#pragma once

#include <memory>
#include <mysql.h>
#include <Mantids30/DB/sqlconnector.h>
#include "query_mariadb.h"

namespace Mantids30 { namespace Database {

/**
 * @brief The SQLConnector_MariaDB class provides an implementation of the SQLConnector interface for the MariaDB database.
 *
 * The SQLConnector_MariaDB class provides an implementation of the SQLConnector interface for the MariaDB database. It allows applications to connect to, query, and manipulate data stored in a MariaDB database.
 *
 * To use the SQLConnector_MariaDB class, create an instance of the class, call the connect() method to establish a connection to the database, and then call the various methods to execute queries and manipulate data.
 */
class SQLConnector_MariaDB : public SQLConnector
{
public:
    SQLConnector_MariaDB();
    ~SQLConnector_MariaDB();

    /**
     * @brief driverName Returns the name of the database driver ("MARIADB").
     * @return The name of the database driver.
     */
    std::string driverName() { return "MARIADB"; }

    /**
     * @brief isOpen Returns true if the database connection is open, otherwise false.
     * @return True if the database connection is open, otherwise false.
     */
    bool isOpen();

    /**
     * @brief getDatabaseConnector Internal function used by the query to prepare the query with the database handler.
     * @param query The query to prepare.
     */
    void getDatabaseConnector(Query_MariaDB * query);

    /**
     * @brief dbTableExist Checks if a table with the given name exists in the MariaDB database.
     * @param table The name of the table to check.
     * @return True if the table exists, otherwise false.
     */
    bool dbTableExist(const std::string & table);

    /**
     * @brief getEscaped Escapes a string for use in a SQL query.
     * @param v The string to escape.
     * @return The escaped string.
     */
    std::string getEscaped(const std::string &v);

protected:
    /**
     * @brief createQuery0 Creates a new instance of the Query_MariaDB class.
     * @return A new instance of the Query_MariaDB class.
     */
    std::shared_ptr<Query> createQuery0() { return std::make_shared<Query_MariaDB>(); };

    /**
     * @brief connect0 Establishes a connection to the MariaDB database.
     * @return True if the connection was successful, otherwise false.
     */
    bool connect0();

private:
    MYSQL *m_databaseConnectionHandler;    // Handler for the MariaDB database connection.
};

}}

