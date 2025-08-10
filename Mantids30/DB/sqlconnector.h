#pragma once

#include <condition_variable>

#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include "databasecredentials.h"
#include "query.h"

namespace Mantids30 {
namespace Database {

class SQLConnector
{
public:
    SQLConnector() = default;
    virtual ~SQLConnector();

    enum eQueryPTRErrors
    {
        QUERY_READY_OK = 0,
        QUERY_UNINITIALIZED = 1,
        QUERY_FINISHED = 2,
        QUERY_UNABLETOADQUIRELOCK = 3,
        QUERY_SQLCONNECTORFINISHED = 4,
        QUERY_ERRORBINDINGINPUTVARS = 5,
        QUERY_ERRORBINDINGRESULTVARS = 6,
        QUERY_RESULTS_FAILED = 7,
        QUERY_SELECTCOUNT_FAILED = 8,
        QUERY_RESULTS_OK = 100
    };

    struct QueryInstance
    {
        QueryInstance()
        {
            error = QUERY_UNINITIALIZED;
            this->query = nullptr;
        }

        QueryInstance(std::shared_ptr<Query> query)
        {
            error = QUERY_READY_OK;
            this->query = query;
        }
        ~QueryInstance()
        {
            query = nullptr;
            error = QUERY_FINISHED;
        }
        std::string getErrorString()
        {
            switch (error)
            {
            case QUERY_READY_OK:
                return "Ready to execute query";
            case QUERY_UNINITIALIZED:
                return "Query uninitialized";
            case QUERY_FINISHED:
                return "Query instance finished (should not happen)";
            case QUERY_UNABLETOADQUIRELOCK:
                return "Unable to acquire lock";
            case QUERY_SQLCONNECTORFINISHED:
                return "SQL Connector finished";
            case QUERY_ERRORBINDINGINPUTVARS:
                return "Error binding input variables";
            case QUERY_ERRORBINDINGRESULTVARS:
                return "Error binding result variables";
            case QUERY_SELECTCOUNT_FAILED:
                return "Error determining the number of rows in the SELECT statement";
            case QUERY_RESULTS_FAILED:
                return query->getLastSQLError();
            case QUERY_RESULTS_OK:
                return "Query executed successfully";
            }
            return "";
        }

        bool getResultsOK() { return error == QUERY_RESULTS_OK; }

        std::shared_ptr<Query> query;
        eQueryPTRErrors error;
    };

    // Database connection:

    bool connect(const std::string &dbFilePath);

    bool attach(const std::string &dbFilePath, const std::string &schemeName);
    bool detach(const std::string &schemeName);

    bool connect(const std::string &host, const uint16_t &port, const DatabaseCredentials &credentials, const std::string &dbName);

    virtual bool isOpen() = 0;

    // Database Internals:
    virtual bool dbTableExist(const std::string &table) = 0;

    // Connection/Database Information:
    virtual std::string driverName() = 0;

    virtual std::string getEscaped(const std::string &value) = 0;

    std::string getLastSQLError() const;

    //std::queue<std::string> getErrorsAndFlush();

    std::string getDBHostname() const;

    DatabaseCredentials getDBCredentialData() const;

    DatabaseCredentials getDBFullCredentialData() const;

    uint16_t getDBPort() const;

    std::string getDBFilePath() const;

    std::string getDBName() const;

    // TODO: Reconnector thread / Reconnection options.

    // SQL Query:
    std::shared_ptr<Query> createQuery(eQueryPTRErrors *error);
    SQLConnector::QueryInstance createQueryInstance();

    void detachQuery(Query *query);

    // Fast Queries Approach (TODO: deprecate query, only work with qExecute/qSelect):
    /**
     * @brief execute Fast Prepared Query for non-row-return statements (non-select).
     * @param preparedQuery Prepared SQL Query String.
     * @param inputVars Input Vars for the prepared execute. (abstract elements will be deleted after the execute is executed)
     * @return true if succeed.
     */
    bool execute(std::string *lastError, const std::string &preparedQuery, const std::map<std::string, std::shared_ptr<Memory::Abstract::Var>> &inputVars = {});
    bool execute(const std::string &preparedQuery, const std::map<std::string, std::shared_ptr<Memory::Abstract::Var>> &inputVars = {});

    /**
     * @brief qExecute Fast Prepared Query for non-row-return statements. (update/insert/delete)
     * @param preparedQuery Prepared SQL Query String.
     * @param inputVars Input Vars for the prepared query. (abstract elements will be deleted when QueryInstance is destroyed)
     * @param outputVars Output Vars for the step iteration.
     * @return pair of bool and query pointer
     *         if the query suceeed, the boolean will be true and there will be a query pointer.
     *         if the query can't be created, the boolean will be false and the query pointer nullptr.
     *         if the query was created, but can not be executed, the boolean is false, but the query is a valid pointer.
     *         NOTE: when the query is a valid pointer, you should delete/destroy the query.
     */
    SQLConnector::QueryInstance qExecute(const std::string &preparedQuery, const std::map<std::string, std::shared_ptr<Memory::Abstract::Var>> &inputVars = {});

    /**
     * @brief qSelect Fast Prepared Query for row-returning statements. (select)
     * @param preparedQuery Prepared SQL Query String.
     * @param inputVars Input Vars for the prepared query. (abstract elements will be deleted when QueryInstance is destroyed)
     * @param outputVars Output Vars for the step iteration.
     * @return pair of bool and query pointer
     *         if the query suceeed, the boolean will be true and there will be a query pointer.
     *         if the query can't be created, the boolean will be false and the query pointer nullptr.
     *         if the query was created, but can not be executed, the boolean is false, but the query is a valid pointer.
     *         NOTE: when the query is a valid pointer, you should delete/destroy the query.
     */
    SQLConnector::QueryInstance qSelect(const std::string &preparedQuery, const std::map<std::string, std::shared_ptr<Memory::Abstract::Var>> &inputVars,
                                                         const std::vector<Memory::Abstract::Var *> &resultVars);

    /**
     * @brief qSelectWithFilters Fast Prepared Query for row-returning statements with additional filters.
     * @param preparedQuery Prepared SQL Query String.
     * @param whereFilters WHERE clause filters to be applied.
     * @param inputVars Input Vars for the prepared query. (abstract elements will be deleted when QueryInstance is destroyed)
     * @param resultVars Output Vars for the step iteration.
     * @param orderby ORDER BY clause string.
     * @param limit LIMIT value for the number of rows returned.
     * @param offset OFFSET value to start returning rows from.
     * @return shared pointer to QueryInstance if successful, nullptr otherwise.
     */
    SQLConnector::QueryInstance qSelectWithFilters(const std::string &preparedQuery, const std::string &whereFilters,
                                                                    const std::map<std::string, std::shared_ptr<Memory::Abstract::Var>> &inputVars,
                                                                    const std::vector<Memory::Abstract::Var *> &resultVars, const std::string &orderby, const uint64_t &limit, const uint64_t &offset);

    bool reconnect(unsigned int magic);

    /**
     * @brief getReconnectIntervalSeconds Get Sleep time in seconds between reconnections
     * @return Sleep time in seconds between reconnections
     */
    uint32_t getReconnectIntervalSeconds() const;
    /**
     * @brief setReconnectIntervalSeconds Set Sleep time in seconds between reconnections
     * @param newReconnectIntervalSeconds sleep time in seconds between reconnections
     */
    void setReconnectIntervalSeconds(uint32_t newReconnectIntervalSeconds);

    /**
     * @brief getMaxReconnectionAttempts Get Max reconnection attempts during a failed query
     * @return max reconnection attempts
     */
    uint32_t getMaxReconnectionAttempts() const;
    /**
     * @brief setMaxReconnectionAttempts Set Max reconnection attempts during a failed query
     * @param newMaxReconnectionAttempts max reconnection attempts during a query
     */
    void setMaxReconnectionAttempts(uint32_t newMaxReconnectionAttempts);

    /**
     * @brief getMaxQueryLockMilliseconds Get Max milliseconds to wait in query to adquire database lock
     * @return Max milliseconds to wait in query to adquire database lock (or 0 to wait indefinitely)
     */
    uint64_t getMaxQueryLockMilliseconds() const;
    /**
     * @brief setMaxQueryLockMilliseconds Set Max Milliseconds to wait in query to adquire database lock before failure
     * @param newMaxQueryLockMilliseconds Max milliseconds to wait in query to adquire database lock (or 0 to wait indefinitely)
     */
    void setMaxQueryLockMilliseconds(uint64_t newMaxQueryLockMilliseconds);

    /**
    * @brief Gets the current configuration for throwing exceptions on query failure.
    *
    * This method checks whether the database class is configured to throw a
    * `std::exception` when a query fails. If enabled, it allows the caller to
    * manage errors using C++ exception handling mechanisms.
    *
    * @return true if exceptions are thrown on query failure, false otherwise.
    */
    bool throwCPPErrorOnQueryFailure() const;

    /**
    * @brief Sets the behavior for handling query failures in the database class.
    *
    * Configures whether the database class should throw a `std::exception`
    * when a query fails. If set to true, exceptions will be thrown to indicate
    * an error. If set to false, the class will rely on alternative error
    * reporting methods, such as return codes or logs.
    *
    * @param newThrowCPPErrorOnQueryFailure Set to true to enable exceptions
    * on query failure, or false to disable them.
    */
    void setThrowCPPErrorOnQueryFailure(bool newThrowCPPErrorOnQueryFailure);

protected:
    virtual std::shared_ptr<Query> createQuery0() { return nullptr; };
    virtual bool connect0() { return false; }
    virtual bool attach0(const std::string &dbFilePath, const std::string &schemeName) { return false; }
    virtual bool detach0(const std::string &schemeName) { return false; }

    std::string m_dbFilePath;

    uint16_t m_port = 0;
    std::string m_host;
    std::string m_dbName;
    DatabaseCredentials m_credentials;

    uint64_t m_maxQueryLockMilliseconds = 10000;
    uint32_t m_reconnectIntervalSeconds = 3;
    uint32_t m_maxReconnectionAttempts = 10;

    std::string m_lastSQLError;

private:
    bool attachQuery(Query *query);

    std::set<Query *> m_querySet;
    bool m_finalized = false;
    bool m_throwCPPErrorOnQueryFailure = false;
    std::mutex m_querySetMutex;
    std::timed_mutex m_databaseLockMutex;

    std::condition_variable m_emptyQuerySetCondition; // Condition variable used to wait for an empty query set.
};

} // namespace Database
} // namespace Mantids30
