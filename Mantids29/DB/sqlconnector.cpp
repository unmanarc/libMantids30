#include "sqlconnector.h"
#include <memory>
#include <unistd.h>

using namespace Mantids29::Database;

SQLConnector::SQLConnector()
{
    m_maxQueryLockMilliseconds = 10000;
    m_maxReconnectionAttempts = 10;
    m_reconnectIntervalSeconds = 3;
    m_finalized = false;
    m_port = 0;
}

SQLConnector::~SQLConnector()
{
    std::unique_lock<std::mutex> lock(this->m_querySetMutex);
    // Disable new queries.
    m_finalized = true;
    // Wait until current queries are finalized.
    while ( !m_querySet.empty() )
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

std::string SQLConnector::getDBHostname() const
{
    return m_host;
}

AuthData SQLConnector::getDBCredentialData() const
{
    AuthData x = m_auth;
    // Remove password ;)
    x.password = "";
    return x;
}

AuthData SQLConnector::getDBFullCredentialData() const
{
    return m_auth;
}

uint16_t SQLConnector::getDBPort() const
{
    return m_port;
}

std::string SQLConnector::getDBFilePath() const
{
    return m_dbFilePath;
}

Query *SQLConnector::createQuery(eQueryPTRErrors *error)
{
    Query * query = createQuery0();
    if (!query) return nullptr;

    if (!attachQuery(query))
    {
        // Query no attached, destroying it...
        *error = QUERY_SQLCONNECTORFINISHED;
        delete query;
        return nullptr;
    }

    if (!query->setSqlConnector(this, &m_databaseLockMutex, m_maxQueryLockMilliseconds))
    {
        // Query will be detached by itself...
        *error = QUERY_UNABLETOADQUIRELOCK;
        delete query;
        return nullptr;
    }

    *error = QUERY_READY_OK;
    return query;
}

std::shared_ptr<SQLConnector::QueryInstance> SQLConnector::createQuerySharedPTR()
{
    eQueryPTRErrors error;
    auto q = std::make_shared<QueryInstance>(createQuery(&error));
    q->error = error;
    return q;
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

bool SQLConnector::query(std::string *lastError, const std::string &preparedQuery, const std::map<std::string, Memory::Abstract::Var *> &inputVars)
{
    auto i = qInsert(preparedQuery,inputVars);
    if (!(i->getResultsOK()))
    {
        *lastError = i->getErrorString();
    }
    return i->getResultsOK();
}

bool SQLConnector::query(const std::string &preparedQuery, const std::map<std::string, Memory::Abstract::Var *> &inputVars)
{
    auto i = qInsert(preparedQuery,inputVars);
    return i->getResultsOK();
}

std::shared_ptr<SQLConnector::QueryInstance> SQLConnector::qSelect(const std::string &preparedQuery, const std::map<std::string, Mantids29::Memory::Abstract::Var *> &inputVars, const std::vector<Mantids29::Memory::Abstract::Var *> &resultVars)
{
    std::shared_ptr<SQLConnector::QueryInstance> q = createQuerySharedPTR();

    if (q->error != QUERY_READY_OK)
        return q;

    if (q->query->setPreparedSQLQuery(preparedQuery,inputVars))
    {
        if (q->query->bindResultVars(resultVars))
        {
            q->error = q->query->exec(Query::EXEC_TYPE_SELECT) ? QUERY_RESULTS_OK : QUERY_RESULTS_FAILED;
            return q;
        }
        else
            q->error = QUERY_ERRORBINDINGRESULTVARS;
    }
    else
        q->error = QUERY_ERRORBINDINGINPUTVARS;
    return q;
}

std::shared_ptr<SQLConnector::QueryInstance> SQLConnector::qInsert(const std::string &preparedQuery, const std::map<std::string, Memory::Abstract::Var *> &inputVars, const std::vector<Memory::Abstract::Var *> &resultVars)
{
    std::shared_ptr<SQLConnector::QueryInstance> q = createQuerySharedPTR();

    if (q->error != QUERY_READY_OK)
        return q;

    if (q->query->setPreparedSQLQuery(preparedQuery,inputVars))
    {
        q->error = q->query->exec(Query::EXEC_TYPE_INSERT) ?  QUERY_RESULTS_OK : QUERY_RESULTS_FAILED;
        return q;
    }
    else
        q->error = QUERY_ERRORBINDINGINPUTVARS;

    return q;
}

bool SQLConnector::attachQuery(Query *query)
{
    std::unique_lock<std::mutex> lock(this->m_querySetMutex);
    if (m_finalized)
        return false;
    m_querySet.insert(query);
    return true;
}

bool Mantids29::Database::SQLConnector::connect(const std::string &host, const uint16_t &port, const Mantids29::Database::AuthData &auth, const std::string &dbName)
{
    this->m_host = host;
    this->m_port = port;
    this->m_auth = auth;
    this->m_dbName = dbName;
    return connect0();
}

std::string SQLConnector::getLastSQLError() const
{
    return m_lastSQLError;
}

/*
std::shared_ptr<SQLConnector::QueryInstance> SQLConnector::query(const std::string &preparedQuery, const std::map<std::string, Memory::Abstract::Var *> &inputVars, const std::vector<Memory::Abstract::Var *> &resultVars)
{
    return qSelect(preparedQuery,inputVars,resultVars);
}*/

bool SQLConnector::reconnect(unsigned int magic)
{
    if (magic == 0xFFFFABCD)
    {
        bool connected = false;
        for (uint32_t i =0; (!m_maxReconnectionAttempts || i<m_maxReconnectionAttempts) && connected == false ; i++)
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
