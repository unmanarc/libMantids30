#pragma once

#include <Mantids30/Memory/a_var.h>
#include <map>
#include <list>
#include <memory>
#include <vector>
#include <mutex>
#include <string>

/**
 * @namespace Mantids30::Database
 * @brief Namespace containing database-related classes and functionality.
 */
namespace Mantids30 { namespace Database {

/**
 * @class Query
 * @brief Represents a database query with support for prepared statements and binding input/output variables.
 *
 * The Query class provides an interface for executing database queries with support for
 * prepared statements and binding input/output variables. It also handles error management
 * and provides access to the results of executed queries.
 */
class Query
{
public:

    /**
     * @enum ExecType
     * @brief Specifies the type of query execution: SELECT or INSERT (incl. UPDATE).
     */
    enum ExecType
    {
        EXEC_TYPE_SELECT,
        EXEC_TYPE_INSERT
    };

    /**
     * @brief Default constructor.
     */
    Query() = default;
    /**
     * @brief Destructor.
     */
    virtual ~Query();

    /**
     * @brief (Internal use) Sets the SQL connector, database lock mutex, and timeout.
     * @param value A pointer to the SQL connector object.
     * @param mtDatabaseLockMutex A pointer to the timed mutex used for database locking.
     * @param milliseconds The number of milliseconds for the lock timeout.
     * @return True if successful, false otherwise.
     */
    bool setSqlConnector(void *value, std::timed_mutex * mtDatabaseLockMutex, const uint64_t & milliseconds);

    // Query Prepare:
    /**
     * @brief Prepares the SQL query.
     * @param value The prepared SQL query string.
     * @param vars Optional input variables to bind.
     * @return True if successful, false otherwise.
     */
    bool setPreparedSQLQuery(const std::string &value, const std::map<std::string,std::shared_ptr<Memory::Abstract::Var>> & vars = {} );
    /**
     * @brief Binds input variables to the prepared SQL query.
     * @param vars The map of input variables to bind.
     * @return True if successful, false otherwise.
     */
    bool bindInputVars(const std::map<std::string,std::shared_ptr<Memory::Abstract::Var>> & vars);
    /**
     * @brief Binds result variables to the prepared SQL query.
     * @param vars The vector of result variables to bind.
     * @return True if successful, false otherwise.
     */
    bool bindResultVars(const std::vector<Mantids30::Memory::Abstract::Var *> &vars);

    // TODO:
    // bool enqueue( void (*_callback)(std::shared_ptr<Query>) = nullptr );

    // Query Execution:
    /**
     * @brief Executes the prepared SQL query.
     * @param execType The type of query execution (SELECT or INSERT).
     * @return True if successful, false otherwise.
     */
    bool exec(const ExecType & execType);

    // GET ROW FROM SELECT Results:
    /**
     * @brief Retrieves the next row from SELECT results.
     * @return True if successful, false otherwise.
     */
    bool step();

    /**
     * @brief Checks if the specified column value is NULL.
     * @param column The index of the column.
     * @return True if NULL, false otherwise.
     */
    bool isNull(const size_t & column);

    /**
    * @brief Retrieves the last inserted row ID.
    * @return The last inserted row ID.
    */
    unsigned long long getLastInsertRowID() const;

    // Error Management:
    /**
    * @brief Retrieves the last SQL error message.
    * @return The last SQL error message as a string.
    */
    std::string getLastSQLError() const;
    /**
    * @brief Retrieves the last SQL return value.
    * @return The last SQL return value as an integer.
    */
    int getLastSQLReturnValue() const;
    /**
     * @brief getNumRows Get the retrieved rows count on SELECT statement
     * warning: does not apply to sqlite3
     * @return number of rows retrieved by the select statement
     */
    uint64_t getNumRows() const;
    /**
     * @brief getAffectedRows Get Affected Rows by INSERT/UPDATE/DELETE commands
     * @return number of affected rows.
     */
    uint64_t getAffectedRows() const;

    bool getFetchLastInsertRowID() const;
    void setFetchLastInsertRowID(bool newFetchLastInsertRowID);

    uint64_t getUnfilteredNumRows() const;
    void setUnfilteredNumRows(uint64_t newUnfilteredNumRows);

protected:
    /**
    * @brief (Internal use) Executes the prepared SQL query.
    * @param execType The type of query execution (SELECT or INSERT).
    * @param recursion A boolean value representing whether the call is recursive.
    * @return True if successful, false otherwise.
    */
    virtual bool exec0(const ExecType & execType, bool recursion)=0;
    /**
    * @brief (Internal use) Retrieves the next row from SELECT results.
    * @return True if successful, false otherwise.
    */
    virtual bool step0() = 0;

    /**
    * @brief (Internal use) Handles post-binding of input variables.
    * @return True if successful, false otherwise.
    */
    virtual bool postBindInputVars() { return true; }
    /**
    * @brief (Internal use) Handles post-binding of result variables.
    * @return True if successful, false otherwise.
    */
    virtual bool postBindResultVars() { return true; }

    bool replaceFirstKey(std::string &sqlQuery, std::list<std::string> &keysIn, std::vector<std::string> &keysOutByPos, const std::string replaceBy);

    std::shared_ptr<std::string> createDestroyableStringForInput(const std::string &str);
    void clearDestroyableStringsForInput();

    std::shared_ptr<std::string> createDestroyableStringForResults(const std::string &str);
    void clearDestroyableStringsForResults();

    // Query:
    bool m_bindInputVars = false;
    bool m_bindResultVars = false;
    std::map<std::string,std::shared_ptr<Memory::Abstract::Var>> m_inputVars;
    std::string m_query;

    // Internals:
    void * m_pSQLConnector = nullptr;

    // Errors:
    std::string m_lastSQLError;
    int m_lastSQLErrno = 0;
    int m_lastSQLReturnValue = 0;

    // Results:
    std::vector<bool> m_fieldIsNull;
    std::vector<Memory::Abstract::Var *> m_resultVars;
    unsigned long long m_lastInsertRowID = 0;
    uint64_t m_numRows = 0;
    uint64_t m_affectedRows = 0;
    uint64_t m_unfilteredNumRows = std::numeric_limits<uint64_t>::max();
    std::timed_mutex * m_databaseLockMutex = nullptr;

    /**
     * @brief m_fetchLastInsertRowID if true, the query will retrieve/update the last inserted RowID. (modify before the query)
     */
    bool m_fetchLastInsertRowID = true;

    bool m_throwCPPErrorOnQueryFailure = false;
private:
    // Memory cleaning:
    std::list<std::shared_ptr<std::string>> m_destroyableStringsForInput, m_destroyableStringsForResults;

    friend class SQLConnector;
};

}}

