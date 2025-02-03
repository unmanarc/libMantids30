#pragma once

#include <Mantids30/DB/query.h>
#include <mysql.h>
#include <vector>

namespace Mantids30 { namespace Database {
/**
 * @brief The Query_MariaDB class provides an implementation of the abstract class Query for MariaDB database.
 */
class Query_MariaDB : public Query
{
public:
    /**
     * @brief Query_MariaDB constructor.
     */
    Query_MariaDB() = default;

    /**
     * @brief ~Query_MariaDB destructor.
     */
    ~Query_MariaDB();

    /**
     * @brief exec Executes a direct SQL query.
     * @param execType Type of execution, such as Select or Insert.
     * @return True if the query was executed successfully, otherwise false.
     */
    bool exec(const ExecType & execType);

    /**
     * @brief mariaDBSetDatabaseConnector Sets the MariaDB database connector for this query.
     * @param dbCnt Pointer to a MYSQL object.
     */
    void mariaDBSetDatabaseConnector( MYSQL *dbCnt );

    /**
     * @brief getLastInsertRowID Gets the last inserted row ID for the query.
     * @return The last inserted row ID.
     */
    my_ulonglong getLastInsertRowID() const;

protected:
    /**
     * @brief exec0 Executes a direct SQL query.
     * @param execType Type of execution, such as Select or Insert.
     * @param recursion Whether or not to perform recursive calls.
     * @return True if the query was executed successfully, otherwise false.
     */
    bool exec0(const ExecType & execType, bool recursion);

    /**
     * @brief step0 Advances the current row of the result set.
     * @return True if the current row was successfully advanced, otherwise false.
     */
    bool step0();

    /**
     * @brief postBindInputVars Binds the input parameters for the prepared statement.
     * @return True if the input parameters were successfully bound, otherwise false.
     */
    bool postBindInputVars();

private:
    /**
     * @brief reconnection Reconnects to the database.
     * @param execType Type of execution, such as Select or Insert.
     * @param recursion Whether or not to perform recursive calls.
     * @return The number of retries that were attempted.
     */
    int reconnection(const ExecType &execType, bool recursion);

    /**
     * @brief connectionError Checks if there was a connection error.
     * @return True if there was a connection error, otherwise false.
     */
    bool connectionError();

    /**
     * @brief mariaDBfetchVarSize Gets the size of a variable from the MariaDB result set.
     * @param col The index of the column.
     * @param fieldType The field type.
     * @return The size of the variable.
     */
    unsigned long mariaDBfetchVarSize(const size_t & col , const enum_field_types &fieldType = MYSQL_TYPE_STRING);

    MYSQL * m_databaseConnectionHandler = nullptr; /**< Pointer to a MYSQL object. */
    MYSQL_STMT * m_stmt = nullptr; /**< Pointer to a MYSQL_STMT object. */
    MYSQL_BIND * m_bindedInputParams = nullptr; /**< Pointer to a MYSQL_BIND object for input parameters. */
    MYSQL_BIND * m_bindedResultsParams = nullptr; /**< Pointer to a MYSQL_BIND object for result set parameters. */
    std::vector<unsigned long> m_bindedResultVarSizes = {}; /**< Vector of variable sizes for the result set. */
    bool m_fetchLastInsertRowID = true; /**< Whether or not to fetch the last inserted row ID. */
    std::vector<std::string> m_keysByPos; /**< Vector of keys for the result set. */
};
}}

