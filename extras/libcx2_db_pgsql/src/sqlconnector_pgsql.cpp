#include "sqlconnector_pgsql.h"
#include <string.h>
#include <cx2_mem_vars/a_string.h>
using namespace CX2::Database;

SQLConnector_PostgreSQL::SQLConnector_PostgreSQL()
{
    conn = nullptr;
    port = 5432;
    uConnectionTimeout = 10;
}

SQLConnector_PostgreSQL::~SQLConnector_PostgreSQL()
{
    if (conn)
        PQfinish(conn);
}

void SQLConnector_PostgreSQL::getDatabaseConnector(Query_PostgreSQL *query)
{
    query->psqlSetDatabaseConnector(conn);
}

bool SQLConnector_PostgreSQL::dbTableExist(const std::string &table)
{
    std::string realTableName;

    if (memchr(table.c_str(),'.',table.size()))
        realTableName = table;
    else
        realTableName = "public." + table;

    // Select Query:
    QueryInstance i = query("SELECT to_regclass(:table);",
                   {
                     { ":table", Memory::Abstract::STRING(realTableName)}
                   },
                   {} );

    if (i.ok)
    {
        if (i.query->step())
        {
            return !i.query->getIsNull(0);
        }
    }
    return false;
}

std::string SQLConnector_PostgreSQL::getEscaped(const std::string &v)
{
    if (!conn)
        return "";
    char cEscaped[(2 * v.size())+1];
    PQescapeStringConn(conn, cEscaped, v.c_str(), v.size(), &psqlEscapeError);
    cEscaped[(2 * v.size())] = 0;
    return cEscaped;
}

bool SQLConnector_PostgreSQL::connect0()
{
    fillConnectionArray();

    char ** ccKeys = getConnectionKeys();
    char ** ccValues = getConnectionValues();

    conn = PQconnectdbParams(ccKeys,ccValues,0);

    destroyArray(ccKeys);
    destroyArray(ccValues);

    if (!conn) return false;

    if (conn)
    {
        if (PQstatus(conn) == CONNECTION_OK)
        {
            return true;
        }
        else
        {
            PQfinish(conn);
            conn = nullptr;
        }
    }

    return false;
}

void SQLConnector_PostgreSQL::fillConnectionArray()
{
    connectionValues.clear();

    if (dbName.size()) connectionValues["dbname"] = dbName;
    if (host.size()) connectionValues["hostname"] = host;

    if (port!=5432) connectionValues["port"] = port;

    if (!auth.getUser().empty())
    {
        connectionValues["user"] = auth.getUser();
        connectionValues["password"] = auth.getPass();
    }

    connectionValues["connect_timeout"] = uConnectionTimeout;

    if (sConnectionOptions.size()) connectionValues["options"] = sConnectionOptions;
    if (sConnectionSSLMode.size()) connectionValues["sslmode"] = sConnectionSSLMode;

}

char **SQLConnector_PostgreSQL::getConnectionKeys()
{
    char ** values = (char **)malloc( (connectionValues.size()+1)*sizeof(char *) );

    uint pos = 0;
    for ( const auto & i : connectionValues )
    {
        values[pos++] = strdup(i.first.c_str());
    }
    values[pos] = nullptr;

    return values;
}

char **SQLConnector_PostgreSQL::getConnectionValues()
{
    char ** values = (char **)malloc( (connectionValues.size()+1)*sizeof(char *) );

    size_t pos = 0;
    for ( const auto & i : connectionValues )
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
    return psqlEscapeError;
}

void SQLConnector_PostgreSQL::psqlSetConnectionTimeout(const uint32_t &value)
{
    uConnectionTimeout = value;
}

void SQLConnector_PostgreSQL::psqlSetConnectionOptions(const std::string &value)
{
    sConnectionOptions = value;
}

void SQLConnector_PostgreSQL::psqlSetConnectionSSLMode(const std::string &value)
{
    sConnectionSSLMode = value;
}
