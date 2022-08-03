#include "sqlconnector.h"
#include <memory>
#include <unistd.h>

using namespace Mantids::Database;

SQLConnector::SQLConnector()
{
    maxReconnectionAttempts = 0;
    reconnectSleepSeconds = 3;
    finalized = false;
    port = 0;
}

SQLConnector::~SQLConnector()
{
    std::unique_lock<std::mutex> lock(this->mtQuerySet);
    // Disable new queries.
    finalized = true;
    // Wait until current queries are finalized.
    while ( !querySet.empty() )
    {
        // Wait for signal when the querySet is empty.
        cvEmptyQuerySet.wait(lock);
    }
}

bool SQLConnector::connect(const std::string &file)
{
    this->dbFilePath = file;
    return connect0();
}

std::string SQLConnector::getDBHostname() const
{
    return host;
}

AuthData SQLConnector::getDBAuthenticationData() const
{
    AuthData x = auth;
    // Remove password ;)
    x.setPass("");
    return x;
}

AuthData SQLConnector::getDBFullAuthenticationData() const
{
    return auth;
}

uint16_t SQLConnector::getDBPort() const
{
    return port;
}

std::string SQLConnector::getDBFilePath() const
{
    return dbFilePath;
}

Query *SQLConnector::prepareNewQuery()
{
    Query * query = createQuery0();
    if (!query) return nullptr;

    if (!attachQuery(query))
    {
        delete query;
        return nullptr;
    }

    query->setSqlConnector(this, &mtDatabaseLock);

    return query;
}

std::shared_ptr<QueryInstance> SQLConnector::prepareNewQueryInstance()
{
    return std::make_shared<QueryInstance>(prepareNewQuery());
}

void SQLConnector::detachQuery(Query *query)
{
    std::unique_lock<std::mutex> lock(this->mtQuerySet);

    // Erase the query from the query set.
    querySet.erase(query);

    // Each time the queryset is empty, emit the proper signal.
    if (querySet.empty())
        cvEmptyQuerySet.notify_all();
}

bool SQLConnector::query(std::string *lastError, const std::string &preparedQuery, const std::map<std::string, Memory::Abstract::Var *> &inputVars)
{
    auto i = qInsert(preparedQuery,inputVars);
    if (lastError && i->query)
    {
        *lastError = "";
        i->query->getLastSQLError();
    }
    return i->ok;
}

bool SQLConnector::query(const std::string &preparedQuery, const std::map<std::string, Memory::Abstract::Var *> &inputVars)
{
    auto i = qInsert(preparedQuery,inputVars);
    return i->ok;
}

std::shared_ptr<QueryInstance> SQLConnector::qInsert(const std::string &preparedQuery, const std::map<std::string, Memory::Abstract::Var *> &inputVars, const std::vector<Memory::Abstract::Var *> &resultVars)
{
    std::shared_ptr<QueryInstance> q = prepareNewQueryInstance();

    if (!q->query)
    {
        q->ok = false;
        return q;
    }

    if (q->query->setPreparedSQLQuery(preparedQuery,inputVars))
    {
        q->ok = q->query->exec(Query::EXEC_TYPE_INSERT);
        return q;
    }

    q->ok = false;
    return q;
}

bool SQLConnector::attachQuery(Query *query)
{
    std::unique_lock<std::mutex> lock(this->mtQuerySet);

    if (finalized) return false;
    querySet.insert(query);
    return true;
}

bool Mantids::Database::SQLConnector::connect(const std::string &host, const uint16_t &port, const Mantids::Database::AuthData &auth, const std::string &dbName)
{
    this->host = host;
    this->port = port;
    this->auth = auth;
    this->dbName = dbName;
    return connect0();
}

std::string SQLConnector::getLastSQLError() const
{
    return lastSQLError;
}

std::shared_ptr<QueryInstance> SQLConnector::qSelect(const std::string &preparedQuery, const std::map<std::string, Mantids::Memory::Abstract::Var *> &inputVars, const std::vector<Mantids::Memory::Abstract::Var *> &resultVars)
{
    std::shared_ptr<QueryInstance> q = prepareNewQueryInstance();

    if (!q->query)
    {
        q->ok = false;
        return q;
    }

    if (q->query->setPreparedSQLQuery(preparedQuery,inputVars))
    {
        if (q->query->bindResultVars(resultVars))
        {
            q->ok = q->query->exec(Query::EXEC_TYPE_SELECT);
            return q;
        }
    }

    q->ok = false;
    return q;
}

std::shared_ptr<QueryInstance> SQLConnector::query(const std::string &preparedQuery, const std::map<std::string, Memory::Abstract::Var *> &inputVars, const std::vector<Memory::Abstract::Var *> &resultVars)
{
    return qSelect(preparedQuery,inputVars,resultVars);
}

bool SQLConnector::reconnect(unsigned int magic)
{
    if (magic == 0xFFFFABCD)
    {
        bool connected = false;
        for (uint32_t i =0; (!maxReconnectionAttempts || i<maxReconnectionAttempts) && connected == false ; i++)
        {
            connected = connect0();
            if (!connected)
                sleep(reconnectSleepSeconds);
        }
        return connected;
    }
    return false;
}

uint32_t SQLConnector::getReconnectSleepSeconds() const
{
    return reconnectSleepSeconds;
}

void SQLConnector::setReconnectSleepSeconds(uint32_t newReconnectSleepSeconds)
{
    reconnectSleepSeconds = newReconnectSleepSeconds;
}

uint32_t SQLConnector::getMaxReconnectionAttempts() const
{
    return maxReconnectionAttempts;
}

void SQLConnector::setMaxReconnectionAttempts(uint32_t newMaxReconnectionAttempts)
{
    maxReconnectionAttempts = newMaxReconnectionAttempts;
}

std::string SQLConnector::getDBName() const
{
    return dbName;
}
