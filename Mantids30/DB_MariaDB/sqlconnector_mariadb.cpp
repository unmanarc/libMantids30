#include "sqlconnector_mariadb.h"
#include <Mantids30/DB/sqlconnector.h>

#include <cstdint>
#include <Mantids30/Memory/a_string.h>
#include <unistd.h>

using namespace Mantids30::Database;

SQLConnector_MariaDB::SQLConnector_MariaDB()
{
    m_databaseConnectionHandler = nullptr;
    m_port = 3306;
}

SQLConnector_MariaDB::~SQLConnector_MariaDB()
{
    if (m_databaseConnectionHandler)
        mysql_close(m_databaseConnectionHandler);
}

bool SQLConnector_MariaDB::isOpen()
{
    if (!m_databaseConnectionHandler) return false;
    std::shared_ptr<SQLConnector::QueryInstance> i = qSelect("SELECT 1;", {},{} );
    if (i->getResultsOK())
        return i->query->step();
    return true;
}

void SQLConnector_MariaDB::getDatabaseConnector(Query_MariaDB *query)
{
    query->mariaDBSetDatabaseConnector(m_databaseConnectionHandler);
}

std::string SQLConnector_MariaDB::getEscaped(const std::string &v)
{
    if (!m_databaseConnectionHandler)
        return "";
    char cEscaped[(2 * v.size())+1];
    mysql_real_escape_string(m_databaseConnectionHandler, cEscaped, v.c_str(), v.size());
    cEscaped[(2 * v.size())] = 0;
    return cEscaped;
}

bool SQLConnector_MariaDB::dbTableExist(const std::string &table)
{
    // Select Query:
    std::shared_ptr<SQLConnector::QueryInstance> i = qSelect("SELECT * FROM information_schema.tables WHERE table_schema=:schema AND table_name=:table LIMIT 1;",
    {
      { ":schema", new Memory::Abstract::STRING(m_dbName)},
      { ":table", new Memory::Abstract::STRING(table)}
    },
    {} );

    if (i->getResultsOK())
        return i->query->step();
    else
        return false;
}

bool SQLConnector_MariaDB::connect0()
{
    if (m_databaseConnectionHandler)
    {
        mysql_close(m_databaseConnectionHandler);
        m_databaseConnectionHandler = nullptr;
    }

    if (m_databaseConnectionHandler == nullptr)
    {
        m_databaseConnectionHandler = mysql_init(nullptr);
        if (m_databaseConnectionHandler == nullptr)
        {
            m_lastSQLError = "mysql_init() failed";
            return false;
        }
    }

    if (mysql_real_connect(m_databaseConnectionHandler, this->m_host.c_str(),
                           this->m_auth.username.c_str(),
                           this->m_auth.password.c_str(),
                           this->m_dbName.c_str(),
                           this->m_port, NULL, 0) == NULL)
    {
        m_lastSQLError = mysql_error(m_databaseConnectionHandler);
        mysql_close(m_databaseConnectionHandler);
        m_databaseConnectionHandler = nullptr;
        return false;
    }

    return true;
}

