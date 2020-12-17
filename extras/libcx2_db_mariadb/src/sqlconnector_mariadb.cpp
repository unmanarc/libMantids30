#include "sqlconnector_mariadb.h"

using namespace CX2::Database;

SQLConnector_MariaDB::SQLConnector_MariaDB()
{
    dbCnt = nullptr;
}

SQLConnector_MariaDB::~SQLConnector_MariaDB()
{
    if (dbCnt)
    {
        mysql_close(dbCnt);
    }
}

bool SQLConnector_MariaDB::prepareQuery(Query_MariaDB *query)
{
    std::unique_lock<std::mutex> lock(mtDatabaseLock);

    return query->mariadbInitSTMT(dbCnt);
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
