#include "sqlconnector_pgsql.h"
#include <string.h>
#include <Mantids30/Memory/a_string.h>
#include <unistd.h>
using namespace Mantids30::Database;

SQLConnector_PostgreSQL::SQLConnector_PostgreSQL()
{
    m_databaseConnectionHandler = nullptr;
    m_port = 5432;
    m_connectionTimeout = 10;
}

SQLConnector_PostgreSQL::~SQLConnector_PostgreSQL()
{
    if (m_databaseConnectionHandler)
        PQfinish(m_databaseConnectionHandler);
}

bool SQLConnector_PostgreSQL::isOpen()
{
    if (!m_databaseConnectionHandler) return false;
    std::shared_ptr<SQLConnector::QueryInstance> i = qSelect("SELECT 1;", {},{} );
    if (i->getResultsOK())
        return i->query->step();
    return true;
}

void SQLConnector_PostgreSQL::getDatabaseConnector(Query_PostgreSQL *query)
{
    query->psqlSetDatabaseConnector(m_databaseConnectionHandler);
}

bool SQLConnector_PostgreSQL::dbTableExist(const std::string &table)
{
    std::string realTableName;

    if (memchr(table.c_str(),'.',table.size()))
        realTableName = table;
    else
        realTableName = "public." + table;

    // Select Query:
    std::shared_ptr<SQLConnector::QueryInstance> i = qSelect("SELECT to_regclass(:table);",
                   {
                     { ":table", new Memory::Abstract::STRING(realTableName)}
                   },
                   {} );

    if (i->getResultsOK())
    {
        if (i->query->step())
        {
            return !i->query->isNull(0);
        }
    }
    return false;
}

std::string SQLConnector_PostgreSQL::getEscaped(const std::string &v)
{
    if (!m_databaseConnectionHandler)
        return "";
    char cEscaped[(2 * v.size())+1];
    PQescapeStringConn(m_databaseConnectionHandler, cEscaped, v.c_str(), v.size(), &m_psqlEscapeError);
    cEscaped[(2 * v.size())] = 0;
    return cEscaped;
}

bool SQLConnector_PostgreSQL::connect0()
{
    if (m_databaseConnectionHandler)
    {
        PQfinish(m_databaseConnectionHandler);
        m_databaseConnectionHandler =nullptr;
    }

    fillConnectionArray();

    char ** ccKeys = getConnectionKeys();
    char ** ccValues = getConnectionValues();

    m_databaseConnectionHandler = PQconnectdbParams(ccKeys,ccValues,0);

    destroyArray(ccKeys);
    destroyArray(ccValues);

    if (!m_databaseConnectionHandler)
        return false;

    if (m_databaseConnectionHandler)
    {
        if (PQstatus(m_databaseConnectionHandler) == CONNECTION_OK)
        {
            return true;
        }
        else
        {
            PQfinish(m_databaseConnectionHandler);
            m_databaseConnectionHandler = nullptr;
        }
    }

    return false;
}

void SQLConnector_PostgreSQL::fillConnectionArray()
{
    m_connectionValues.clear();

    if (m_dbName.size()) m_connectionValues["dbname"] = m_dbName;
    if (m_host.size()) m_connectionValues["hostname"] = m_host;

    if (m_port!=5432) m_connectionValues["port"] = m_port;

    if (!m_auth.username.empty())
    {
        m_connectionValues["user"] = m_auth.username;
        m_connectionValues["password"] = m_auth.password;
    }

    m_connectionValues["connect_timeout"] = m_connectionTimeout;

    if (m_connectionOptions.size()) m_connectionValues["options"] = m_connectionOptions;
    if (m_connectionSSLMode.size()) m_connectionValues["sslmode"] = m_connectionSSLMode;

}

char **SQLConnector_PostgreSQL::getConnectionKeys()
{
    char ** values = (char **)malloc( (m_connectionValues.size()+1)*sizeof(char *) );

    size_t pos = 0;
    for ( const auto & i : m_connectionValues )
    {
        values[pos++] = strdup(i.first.c_str());
    }
    values[pos] = nullptr;

    return values;
}

char **SQLConnector_PostgreSQL::getConnectionValues()
{
    char ** values = (char **)malloc( (m_connectionValues.size()+1)*sizeof(char *) );

    size_t pos = 0;
    for ( const auto & i : m_connectionValues )
    {
        values[pos++] = strdup(i.second.c_str());
    }
    values[pos] = nullptr;

    return values;
}

void SQLConnector_PostgreSQL::destroyArray(char **values)
{
    for ( size_t pos=0;values[pos]!=nullptr;pos++ )
    {
        free(values[pos]);
    }
    free(values);
}


int SQLConnector_PostgreSQL::getPsqlEscapeError() const
{
    return m_psqlEscapeError;
}

void SQLConnector_PostgreSQL::psqlSetConnectionTimeout(const uint32_t &value)
{
    m_connectionTimeout = value;
}

void SQLConnector_PostgreSQL::psqlSetConnectionOptions(const std::string &value)
{
    m_connectionOptions = value;
}

void SQLConnector_PostgreSQL::psqlSetConnectionSSLMode(const std::string &value)
{
    m_connectionSSLMode = value;
}
