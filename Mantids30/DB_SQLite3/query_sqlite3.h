#pragma once

#include <Mantids30/DB/query.h>
#include <sqlite3.h>

namespace Mantids30::Database {
/**
 * @brief The Query_SQLite3 class
 *
 * This class represents a query object for SQLite3 databases. It extends the base Query class and adds
 * SQLite3 specific functions.
 */
class Query_SQLite3 : public Query
{
public:
    Query_SQLite3();
    ~Query_SQLite3();

    /**
     * @brief sqlite3IsDone Checks if the query is done.
     * @return true if the query is done, false otherwise.
     */
    bool sqlite3IsDone() const;

    /**
     * @brief setDatabaseConnectionHandler Sets the SQLite3 database connection handler.
     * @param newPpDb Pointer to the SQLite3 database connection handler.
     */
    void setDatabaseConnectionHandler(sqlite3 *newPpDb);

protected:
    /**
     * @brief exec0 Executes the query.
     * @param execType The type of the execution (e.g. SELECT, INSERT, UPDATE, etc.).
     * @param recursion Whether the execution is recursive or not.
     * @return true if the execution is successful, false otherwise.
     */
    bool exec0(const ExecType &execType, bool recursion);

    /**
     * @brief step0 Advances the query to the next row.
     * @return true if the operation is successful, false otherwise.
     */
    bool step0();

private:
    sqlite3_stmt *m_stmt;                 ///< Pointer to the SQLite3 statement object.
    sqlite3 *m_databaseConnectionHandler; ///< Pointer to the SQLite3 database connection handler.
};

} // namespace Mantids30::Database
