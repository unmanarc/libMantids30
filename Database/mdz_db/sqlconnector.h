#ifndef SQLCONNECTOR_H
#define SQLCONNECTOR_H

#include <condition_variable>
#include <mutex>
#include <string>
#include <queue>
#include <set>

#include "authdata.h"
#include "query.h"

namespace Mantids { namespace Database {

struct QueryInstance {
    QueryInstance( Query * query )
    {
        ok = true;
        this->query = query;
    }
    ~QueryInstance()
    {
        if (query) delete query;
    }
     Query * query;
     bool ok;
};

class SQLConnector
{
public:
    SQLConnector();
    virtual ~SQLConnector();


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
    Query * prepareNewQuery();
    QueryInstance prepareNewQueryInstance();

    void detachQuery( Query * query );


    // Fast Queries Approach:
    /**
     * @brief query Fast Prepared Query for non-return statements (non-select).
     * @param preparedQuery Prepared SQL Query String.
     * @param inputVars Input Vars for the prepared query. (abstract elements will be deleted after the query is executed)
     * @return true if succeed.
     */
    bool query(std::string * lastError, const std::string & preparedQuery, const std::map<std::string, Memory::Abstract::Var *> &inputVars = {} );

    bool query(const std::string & preparedQuery, const std::map<std::string, Memory::Abstract::Var *> &inputVars = {} );

    /**
     * @brief query Fast Prepared Query for row-returning statements. (select)
     * @param preparedQuery Prepared SQL Query String.
     * @param inputVars Input Vars for the prepared query. (abstract elements will be deleted when QueryInstance is destroyed)
     * @param outputVars Output Vars for the step iteration.
     * @return pair of bool and query pointer
     *         if the query suceeed, the boolean will be true and there will be a query pointer.
     *         if the query can't be created, the boolean will be false and the query pointer nullptr.
     *         if the query was created, but can not be executed, the boolean is false, but the query is a valid pointer.
     *         NOTE: when the query is a valid pointer, you should delete/destroy the query.
     */
    QueryInstance query(const std::string & preparedQuery,
                const std::map<std::string,Memory::Abstract::Var *> & inputVars,
                const std::vector<Memory::Abstract::Var *> & resultVars
                );

protected:
    virtual Query * createQuery0() { return nullptr; };
    virtual bool connect0() { return false; }

    std::string dbFilePath;
    std::string host;
    std::string dbName;
    uint16_t port;
    AuthData auth;

    std::string lastSQLError;

private:
    bool attachQuery( Query * query );

    std::set<Query *> querySet;
    bool finalized;
    std::mutex mtQuerySet;
    std::mutex mtDatabaseLock;

    std::condition_variable cvEmptyQuerySet;
};

}}

#endif // SQLCONNECTOR_H
