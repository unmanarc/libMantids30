#pragma once

#include "query_sqlite3.h"
#include <Mantids30/DB/sqlconnector.h>
#include <sqlite3.h>

namespace Mantids30::Database {

class SQLConnector_SQLite3 : public SQLConnector
{
public:
    enum class PragmaSyncMode : uint8_t
    {
        OFF = 0,
        NORMAL = 1,
        FULL = 2,
        EXTRA = 3
    };

    enum class PragmaJournalMode : uint8_t
    {
        OFF,
        WAL,
        MEMORY,
        PERSIST,
        TRUNCATE,
        DELETE
    };

    SQLConnector_SQLite3() = default;
    ~SQLConnector_SQLite3() override;

    /**
     * @brief connectInMemory Connect to IN-MEMORY SQLite3 Database (like using connect(":memory:"))
     * reference: https://sqlite.org/inmemorydb.html
     * @return true if connected
     */
    bool connectInMemory();

    /**
     * @brief isOpen Check if the database is open
     * @return true if open.
     */
    [[nodiscard]] bool isOpen() override;

    /**
     * @brief driverName Get driver Name.
     * @return driver name (SQLITE3)
     */
    [[nodiscard]] std::string driverName() override { return "SQLITE3"; }

    /**
     * @brief dbTableExist Check if sqlite3 table exist
     * @param table table name
     * @return true if exist, otherwise false.
     */
    [[nodiscard]] bool dbTableExist(const std::string &table) override;

    /**
     * @brief prepareQuery Internal function used by the query to prepare the query with the database handler.
     * @param query Query.
     * @return true if succeed.
     */
    void putDatabaseConnectorIntoQuery(Query_SQLite3 *query);

    bool setPragmaForeignKeys(bool on = true);
    bool setPragmaJournalMode(const PragmaJournalMode &mode);
    bool setPragmaSynchronous(const PragmaSyncMode &mode);

    [[nodiscard]] std::string getEscaped(const std::string &value) override;

    bool beginTransaction() override;
    bool rollbackTransaction() override;
    bool commitTransaction() override;

protected:
    std::shared_ptr<Query> createQuery0() override { return std::make_shared<Query_SQLite3>(); }
    bool connect0() override;
    bool attach0(const std::string &dbFilePath, const std::string &schemeName) override;
    bool detach0(const std::string &schemeName) override;

private:
    int m_rc = 0;
    sqlite3 *m_ppDb = nullptr;
};
} // namespace Mantids30::Database
