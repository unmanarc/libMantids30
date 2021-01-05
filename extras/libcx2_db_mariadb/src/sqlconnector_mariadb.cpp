#include "sqlconnector_mariadb.h"

#include <cx2_mem_vars/a_string.h>

using namespace CX2::Database;

SQLConnector_MariaDB::SQLConnector_MariaDB()
{
    dbCnt = nullptr;
    port = 3306;
}

SQLConnector_MariaDB::~SQLConnector_MariaDB()
{
    if (dbCnt)
        mysql_close(dbCnt);
}

void SQLConnector_MariaDB::getDatabaseConnector(Query_MariaDB *query)
{
    query->mariaDBSetDatabaseConnector(dbCnt);
}

std::string SQLConnector_MariaDB::getEscaped(const std::string &v)
{
    if (!dbCnt)
        return "";
    char cEscaped[(2 * v.size())+1];
    mysql_real_escape_string(dbCnt, cEscaped, v.c_str(), v.size());
    cEscaped[(2 * v.size())] = 0;
    return cEscaped;
}

bool SQLConnector_MariaDB::dbTableExist(const std::string &table)
{
    // Select Query:
    QueryInstance i = query("SELECT * FROM information_schema.tables WHERE table_schema = :schema AND table_name = :table LIMIT 1;",
                   {
                     { ":schema", new Memory::Abstract::STRING(dbName)},
                     { ":table", new Memory::Abstract::STRING(table)}
                   },
                   {} );

    if (i.ok)
        return i.query->step();
    else
        return false;
}

bool SQLConnector_MariaDB::connect0()
{
    dbCnt = mysql_init(nullptr);
    if (dbCnt == nullptr)
    {
        lastSQLError = "mysql_init() failed";
        return false;
    }

    if (mysql_real_connect(dbCnt, this->host.c_str(),
                           this->auth.getUser().c_str(),
                           this->auth.getPass().c_str(),
                           this->dbName.c_str(),
                           this->port, NULL, 0) == NULL)
    {
        lastSQLError = mysql_error(dbCnt);
        mysql_close(dbCnt);
        dbCnt = nullptr;
        return false;
    }

    return true;

}
