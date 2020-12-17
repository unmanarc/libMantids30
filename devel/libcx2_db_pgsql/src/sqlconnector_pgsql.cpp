#include "sqlconnector_pgsql.h"

using namespace CX2::Database;

SQLConnector_PostgreSQL::SQLConnector_PostgreSQL()
{
    conn = nullptr;
}

SQLConnector_PostgreSQL::~SQLConnector_PostgreSQL()
{

}

bool SQLConnector_PostgreSQL::connect0()
{
    //conn = PQconnectdb();

}
