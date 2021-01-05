#include "sqlconnector.h"

using namespace CX2::Database;

SQLConnector::SQLConnector()
{
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

QueryInstance SQLConnector::prepareNewQueryInstance()
{
    return QueryInstance(prepareNewQuery());
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

bool SQLConnector::attachQuery(Query *query)
{
    std::unique_lock<std::mutex> lock(this->mtQuerySet);

    if (finalized) return false;
    querySet.insert(query);
    return true;
}

bool CX2::Database::SQLConnector::connect(const std::string &host, const uint16_t &port, const CX2::Database::AuthData &auth, const std::string &dbName)
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

bool SQLConnector::query(const std::string &preparedQuery, const std::map<std::string, CX2::Memory::Abstract::Var *> &inputVars)
{
    QueryInstance q = prepareNewQueryInstance();
    if (!q.query)
        return false;
    if (!q.query->setPreparedSQLQuery(preparedQuery, inputVars))
        return false;
    return q.query->exec(EXEC_TYPE_INSERT);
}

QueryInstance SQLConnector::query(const std::string &preparedQuery, const std::map<std::string, CX2::Memory::Abstract::Var *> &inputVars, const std::vector<CX2::Memory::Abstract::Var *> &resultVars)
{
    QueryInstance q = prepareNewQueryInstance();

    if (!q.query)
    {
        q.ok = false;
        return q;
    }

    if (q.query->setPreparedSQLQuery(preparedQuery,inputVars))
    {
        if (q.query->bindResultVars(resultVars))
        {
            q.ok = q.query->exec(EXEC_TYPE_SELECT);
            return q;
        }
    }

    q.ok = false;
    return q;
}

std::string SQLConnector::getDBName() const
{
    return dbName;
}
