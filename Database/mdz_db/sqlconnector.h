#ifndef SQLCONNECTOR_H
#define SQLCONNECTOR_H

#include <condition_variable>

#include <cstdint>
#include <mutex>
#include <string>
#include <queue>
#include <set>
#include <memory>

#include "authdata.h"
#include "query.h"

namespace Mantids { namespace Database {

class SQLConnector
{
public:
    SQLConnector();
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
        QUERY_RESULTS_OK = 8
    };


    struct QueryInstance {
        QueryInstance()
        {
            error = QUERY_UNINITIALIZED;
            this->query = nullptr;
        }

        QueryInstance( Query * query )
        {
            error = QUERY_READY_OK;
            this->query = query;
        }
        ~QueryInstance()
        {
            if (query)
                delete query;
            query = nullptr;
            error = QUERY_FINISHED;
        }
        std::string getErrorString()
        {
            switch(error)
            {
            case QUERY_READY_OK:
                return "Ready to execute query";
            case QUERY_UNINITIALIZED:
                return "Query Uninitalized";
            case QUERY_FINISHED:
                return "Query Instance Finished (should not happen)";
            case QUERY_UNABLETOADQUIRELOCK:
                return "Unable to adquire lock";
            case QUERY_SQLCONNECTORFINISHED:
                return "SQL Connector Finished";
            case QUERY_ERRORBINDINGINPUTVARS:
                return "Error binding input variables";
            case QUERY_ERRORBINDINGRESULTVARS:
                return "Error binding the result variables";
            case QUERY_RESULTS_FAILED:
                return query->getLastSQLError();
            case QUERY_RESULTS_OK:
                return "Query Executed";
            }
            return "";
        }

        bool getResultsOK()
        {
            return error == QUERY_RESULTS_OK;
        }

        Query * query;
        eQueryPTRErrors error;
    };

    // Database connection:

    bool connect( const std::string &dbFilePath );

    bool connect( const std::string &host,
                  const uint16_t & port,
                  const AuthData & auth,
                  const std::string & dbName
                  );

    virtual bool isOpen() = 0;

    // Database Internals:
    virtual bool dbTableExist(const std::string & table) = 0;

    // Connection/Database Information:
    virtual std::string driverName() = 0;

    virtual std::string getEscaped(const std::string & value) = 0;

    std::string getLastSQLError() const;

    //std::queue<std::string> getErrorsAndFlush();

    std::string getDBHostname() const;

    AuthData getDBAuthenticationData() const;

    AuthData getDBFullAuthenticationData() const;

    uint16_t getDBPort() const;

    std::string getDBFilePath() const;

    std::string getDBName() const;

    // TODO: Reconnector thread / Reconnection options.

    // SQL Query:
    Query * createQuery(eQueryPTRErrors * error);
    std::shared_ptr<SQLConnector::QueryInstance> createQuerySharedPTR();

    void detachQuery( Query * query );

    // Fast Queries Approach (TODO: deprecate query, only work with qInsert/qSelect):
    /**
     * @brief query Fast Prepared Query for non-row-return statements (non-select).
     * @param preparedQuery Prepared SQL Query String.
     * @param inputVars Input Vars for the prepared query. (abstract elements will be deleted after the query is executed)
     * @return true if succeed.
     */
    bool query(std::string * lastError, const std::string & preparedQuery, const std::map<std::string, Memory::Abstract::Var *> &inputVars = {} );
    bool query(const std::string & preparedQuery, const std::map<std::string, Memory::Abstract::Var *> &inputVars = {} );

    /**
     * @brief qInsert Fast Prepared Query for non-row-return statements. (update/insert/delete)
     * @param preparedQuery Prepared SQL Query String.
     * @param inputVars Input Vars for the prepared query. (abstract elements will be deleted when QueryInstance is destroyed)
     * @param outputVars Output Vars for the step iteration.
     * @return pair of bool and query pointer
     *         if the query suceeed, the boolean will be true and there will be a query pointer.
     *         if the query can't be created, the boolean will be false and the query pointer nullptr.
     *         if the query was created, but can not be executed, the boolean is false, but the query is a valid pointer.
     *         NOTE: when the query is a valid pointer, you should delete/destroy the query.
     */
    std::shared_ptr<SQLConnector::QueryInstance> qInsert(const std::string & preparedQuery,
                                                         const std::map<std::string,Memory::Abstract::Var *> & inputVars = {},
                                                         const std::vector<Memory::Abstract::Var *> & resultVars = {}
                                                        );

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
    std::shared_ptr<SQLConnector::QueryInstance> qSelect(const std::string & preparedQuery,
                                           const std::map<std::string,Memory::Abstract::Var *> & inputVars,
                                           const std::vector<Memory::Abstract::Var *> & resultVars
                                           );
    /*std::shared_ptr<SQLConnector::QueryInstance> query(const std::string & preparedQuery,
                                         const std::map<std::string,Memory::Abstract::Var *> & inputVars,
                                         const std::vector<Memory::Abstract::Var *> & resultVars
                                         );*/

    bool reconnect(unsigned int magic);


    /**
     * @brief getReconnectSleepSeconds Get Sleep time in seconds between reconnections
     * @return Sleep time in seconds between reconnections
     */
    uint32_t getReconnectSleepSeconds() const;
    /**
     * @brief setReconnectSleepSeconds Set Sleep time in seconds between reconnections
     * @param newReconnectSleepSeconds sleep time in seconds between reconnections
     */
    void setReconnectSleepSeconds(uint32_t newReconnectSleepSeconds);

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

protected:
    virtual Query * createQuery0() { return nullptr; };
    virtual bool connect0() { return false; }

    std::string dbFilePath;
    std::string host;
    std::string dbName;
    AuthData auth;

    uint64_t maxQueryLockMilliseconds;
    uint16_t port;
    uint32_t reconnectSleepSeconds;
    uint32_t maxReconnectionAttempts;

    std::string lastSQLError;

private:
    bool attachQuery( Query * query );

    std::set<Query *> querySet;
    bool finalized;
    std::mutex mtQuerySet;
    std::timed_mutex mtDatabaseLock;

    std::condition_variable cvEmptyQuerySet;
};

}}

#endif // SQLCONNECTOR_H
