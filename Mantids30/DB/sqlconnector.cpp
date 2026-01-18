#include "sqlconnector.h"
#include "query.h"
#include <Mantids30/Memory/a_int64.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <memory>
#include <unistd.h>

using namespace Mantids30::Database;

SQLConnector::~SQLConnector()
{
    std::unique_lock<std::mutex> lock(this->m_querySetMutex);
    // Disable new queries.
    m_finalized = true;
    // Wait until current queries are finalized.
    while (!m_querySet.empty())
    {
        // Wait for signal when the querySet is empty.
        m_emptyQuerySetCondition.wait(lock);
    }
}

bool SQLConnector::connect(const std::string &file)
{
    this->m_dbFilePath = file;
    return connect0();
}

bool SQLConnector::attach(const std::string &dbFilePath, const std::string &schemeName)
{
    return attach0(dbFilePath, schemeName);
}

bool SQLConnector::detach(const std::string &schemeName)
{
    return detach0(schemeName);
}

std::string SQLConnector::getDBHostname() const
{
    return m_host;
}

DatabaseCredentials SQLConnector::getDBCredentialData() const
{
    DatabaseCredentials x = m_credentials;
    // Remove password ;)
    x.userPassword = "";
    return x;
}

DatabaseCredentials SQLConnector::getDBFullCredentialData() const
{
    return m_credentials;
}

uint16_t SQLConnector::getDBPort() const
{
    return m_port;
}

std::string SQLConnector::getDBFilePath() const
{
    return m_dbFilePath;
}

std::shared_ptr<Query> SQLConnector::createQuery()
{
    std::shared_ptr<Query> query = createQuery0();
    if (!query)
        return nullptr;

    if (!attachQuery(query.get()))
    {
        // Query no attached, destroying it...
        query->setError(Query::QUERY_SQLCONNECTORFINISHED);
        return query;
    }

    query->m_throwCPPErrorOnQueryFailure = m_throwCPPErrorOnQueryFailure;
    query->m_throwCPPErrorOnUniqueFailure = m_throwCPPErrorOnUniqueFailure;

    if (!query->setSqlConnector(this, &m_databaseLockMutex, m_maxQueryLockMilliseconds))
    {
        // Query will be detached by itself...
        query->setError(Query::QUERY_UNABLETOADQUIRELOCK);
        return query;
    }

    query->setError(Query::QUERY_READY_OK);
    return query;
}

void SQLConnector::detachQuery(Query *query)
{
    std::unique_lock<std::mutex> lock(this->m_querySetMutex);

    // Erase the query from the query set.
    m_querySet.erase(query);

    // Each time the queryset is empty, emit the proper signal.
    if (m_querySet.empty())
        m_emptyQuerySetCondition.notify_all();
}

std::shared_ptr<Query> SQLConnector::qSelect(const std::string &preparedQuery, const std::map<std::string, std::shared_ptr<Mantids30::Memory::Abstract::Var>> &inputVars,
                                                                   const std::vector<Mantids30::Memory::Abstract::Var *> &resultVars)
{
    std::shared_ptr<Query> q = createQuery();

    if (q && q->getError() != Query::QUERY_READY_OK)
        return q;

    if (q->setPreparedSQLQuery(preparedQuery, inputVars))
    {
        if (q->bindResultVars(resultVars))
        {
            q->setError(q->exec(Query::EXEC_TYPE_SELECT) ? Query::QUERY_RESULTS_OK : Query::QUERY_RESULTS_FAILED);
            return q;
        }
        else
            q->error = Query::QUERY_ERRORBINDINGRESULTVARS;
    }
    else
        q->error = Query::QUERY_ERRORBINDINGINPUTVARS;

    return q;
}

std::shared_ptr<Query> SQLConnector::qSelectWithFilters(std::string preparedQuery, const std::string &whereFilters,
                                                                              const std::map<std::string, std::shared_ptr<Memory::Abstract::Var>> &inputVars,
                                                                              const std::vector<Memory::Abstract::Var *> &resultVars, const std::string &orderby, const uint64_t &limit,
                                                                              const uint64_t &offset)
{
    boost::algorithm::trim(preparedQuery);

    // Remove trailing semicolon if present
    if (!preparedQuery.empty() && preparedQuery.back() == ';')
    {
        preparedQuery.pop_back();
    }

    // Check if WHERE clause exists in the preparedQuery using boost::algorithm
    if (!boost::algorithm::icontains(preparedQuery, "WHERE"))
    {
        preparedQuery+= " WHERE 1=1";
    }

    // First phase: get the count of records without filters and limits
    std::string unfilteredTotalCountQuery = "SELECT COUNT(*) FROM (" + preparedQuery + ") AS subquery";
    std::string filteredTotalCountQuery = "SELECT COUNT(*) FROM (" + preparedQuery + ") AS subquery";

    if (!whereFilters.empty())
    {
        boost::ireplace_last( filteredTotalCountQuery, "WHERE ", "WHERE (" + whereFilters + ") AND " );
    }


    Memory::Abstract::INT64 unfilteredTotalCount,filteredTotalCount;
    {
        std::shared_ptr<Query> countResultQuery = qSelect(unfilteredTotalCountQuery, inputVars, {&unfilteredTotalCount});

        if (!countResultQuery)
            return countResultQuery;

        if (!countResultQuery->isSuccessful() || !countResultQuery->step())
        {
            countResultQuery->setError( Query::QUERY_SELECTCOUNT_FAILED );
            return countResultQuery;
        }
    }
    {
        std::shared_ptr<Query> countResultQuery = qSelect(filteredTotalCountQuery, inputVars, {&filteredTotalCount});

        if (!countResultQuery)
            return countResultQuery;


        if (!countResultQuery->isSuccessful() || !countResultQuery->step())
        {
            countResultQuery->setError( Query::QUERY_SELECTCOUNT_FAILED );
            return countResultQuery;
        }
    }

    // Second phase: build the full query with filters and limits
    std::string fullQuery =  preparedQuery;
    if (!whereFilters.empty())
    {
        boost::ireplace_last( fullQuery, "WHERE ", "WHERE (" + whereFilters + ") AND " );
    }

    if (!orderby.empty())
    {
        fullQuery += "\n ORDER BY " + orderby;
    }

    if (limit > 0)
    {
        fullQuery += "\n LIMIT " + std::to_string(limit);
        if (offset > 0)
        {
            fullQuery += "\n OFFSET " + std::to_string(offset);
        }
    }

    std::shared_ptr<Query> result = qSelect(fullQuery, inputVars, resultVars);

    if (!result)
        return result;

    result->setTotalRecordsCount(unfilteredTotalCount.getValue());
    result->setFilteredRecordsCount(filteredTotalCount.getValue());
    return result;
}

std::shared_ptr<Query> SQLConnector::qExecute(const std::string &preparedQuery, const std::map<std::string, std::shared_ptr<Memory::Abstract::Var>> &inputVars)
{
    std::shared_ptr<Query> q = createQuery();

    if (q && q->getError() != Query::QUERY_READY_OK)
        return q;

    if (q->setPreparedSQLQuery(preparedQuery, inputVars))
    {
        q->error = q->exec(Query::EXEC_TYPE_INSERT) ? Query::QUERY_RESULTS_OK : Query::QUERY_RESULTS_FAILED;
        return q;
    }
    else
        q->error = Query::QUERY_ERRORBINDINGINPUTVARS;

    return q;
}

bool SQLConnector::qSelectSingleRow(const std::string &preparedQuery, const std::map<std::string, std::shared_ptr<Memory::Abstract::Var> > &inputVars, const std::vector<Memory::Abstract::Var *> &resultVars)
{
    auto i = qSelect(preparedQuery,inputVars,resultVars);
    return i && i->isSuccessful() && i->step();
}

bool SQLConnector::attachQuery(Query *query)
{
    std::unique_lock<std::mutex> lock(this->m_querySetMutex);
    if (m_finalized)
        return false;
    m_querySet.insert(query);
    return true;
}

void SQLConnector::setThrowCPPErrorOnUniqueFailure(bool newThrowCPPErrorOnUniqueFailure)
{
    m_throwCPPErrorOnUniqueFailure = newThrowCPPErrorOnUniqueFailure;
}

bool SQLConnector::throwCPPErrorOnUniqueFailure() const
{
    return m_throwCPPErrorOnUniqueFailure;
}

bool SQLConnector::throwCPPErrorOnQueryFailure() const
{
    return m_throwCPPErrorOnQueryFailure;
}

void SQLConnector::setThrowCPPErrorOnQueryFailure(bool newThrowCPPErrorOnQueryFailure)
{
    m_throwCPPErrorOnQueryFailure = newThrowCPPErrorOnQueryFailure;
}

bool Mantids30::Database::SQLConnector::connect(const std::string &host, const uint16_t &port, const Mantids30::Database::DatabaseCredentials &credentials, const std::string &dbName)
{
    this->m_host = host;
    this->m_port = port;
    this->m_credentials = credentials;
    this->m_dbName = dbName;
    return connect0();
}

std::string SQLConnector::getLastSQLError() const
{
    return m_lastSQLError;
}

bool SQLConnector::reconnect(unsigned int magic)
{
    if (magic == 0xFFFFABCD)
    {
        bool connected = false;
        for (uint32_t i = 0; (!m_maxReconnectionAttempts || i < m_maxReconnectionAttempts) && connected == false; i++)
        {
            connected = connect0();
            if (!connected)
                sleep(m_reconnectIntervalSeconds);
        }
        return connected;
    }
    return false;
}

uint32_t SQLConnector::getReconnectIntervalSeconds() const
{
    return m_reconnectIntervalSeconds;
}

void SQLConnector::setReconnectIntervalSeconds(uint32_t newReconnectIntervalSeconds)
{
    m_reconnectIntervalSeconds = newReconnectIntervalSeconds;
}

uint32_t SQLConnector::getMaxReconnectionAttempts() const
{
    return m_maxReconnectionAttempts;
}

void SQLConnector::setMaxReconnectionAttempts(uint32_t newMaxReconnectionAttempts)
{
    m_maxReconnectionAttempts = newMaxReconnectionAttempts;
}

uint64_t SQLConnector::getMaxQueryLockMilliseconds() const
{
    return m_maxQueryLockMilliseconds;
}

void SQLConnector::setMaxQueryLockMilliseconds(uint64_t newMaxQueryLockMilliseconds)
{
    m_maxQueryLockMilliseconds = newMaxQueryLockMilliseconds;
}

std::string SQLConnector::getDBName() const
{
    return m_dbName;
}
