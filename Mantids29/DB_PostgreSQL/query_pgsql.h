#pragma once

#include <Mantids29/DB/query.h>

//#if __has_include(<libpq-fe.h>)
#include <libpq-fe.h>
//#elif __has_include(<postgresql/libpq-fe.h>)
//# include <postgresql/libpq-fe.h>
//#endif
namespace Mantids29 { namespace Database {

/**
 * @brief The Query_PostgreSQL class provides an interface for executing queries on PostgreSQL databases.
 */
class Query_PostgreSQL : public Query
{
public:
    /**
     * @brief Query_PostgreSQL constructor.
     */
    Query_PostgreSQL();

    /**
     * @brief Query_PostgreSQL destructor.
     */
    ~Query_PostgreSQL();

    /**
     * @brief exec Executes a SQL statement.
     * @param execType Type of execution (insert/select).
     * @return true if the query was executed successfully, false otherwise.
     */
    bool exec(const ExecType & execType);

    /**
     * @brief psqlSetDatabaseConnector Sets the database connection handler.
     * @param conn PostgreSQL connection handler.
     */
    void psqlSetDatabaseConnector(PGconn *conn );

    /**
     * @brief psqlGetExecStatus Gets the execution status of the last query executed.
     * @return Execution status of the last query.
     */
    ExecStatusType psqlGetExecStatus() const;

protected:
    /**
     * @brief exec0 Executes a SQL statement and handles errors.
     * @param execType Type of execution (insert/select).
     * @param recursion Whether the function is called recursively.
     * @return true if the query was executed successfully, false otherwise.
     */
    bool exec0(const ExecType & execType, bool recursion);

    /**
     * @brief step0 Advances the query result to the next row.
     * @return true if there is a next row, false otherwise.
     */
    bool step0();

    /**
     * @brief postBindInputVars Processes the input parameters after binding.
     * @return true if the input parameters are processed successfully, false otherwise.
     */
    bool postBindInputVars();

private:
    std::vector<std::string> keysByPos; ///< Map of column names by position.

    size_t m_paramCount; ///< Number of query parameters.
    char ** m_paramValues; ///< Query parameter values.
    int * m_paramLengths; ///< Lengths of query parameter values.
    int * m_paramFormats; ///< Formats of query parameter values.

    ExecStatusType m_execStatus; ///< Execution status of the last query executed.

    PGconn *m_databaseConnectionHandler; ///< PostgreSQL connection handler.
    PGresult* m_results; ///< Query results.
    int m_currentRow; ///< Current row in the query result.
};

}} // Mantids29::Database

