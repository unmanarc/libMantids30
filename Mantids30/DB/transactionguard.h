#pragma once

#include "sqlconnector.h"

/**
 * @namespace Mantids30::Database
 * @brief Namespace containing database-related classes and functionality.
 */
namespace Mantids30::Database {

/// @brief RAII-based transaction manager for database operations.
///
/// TransactionGuard automates the lifecycle of a database transaction using the
/// RAII (Resource Acquisition Is Initialization) idiom. It starts a transaction
/// upon construction, and ensures proper cleanup (rollback) upon destruction if
/// the transaction was not explicitly finalized.
///
/// @par Usage:
/// |// Create a guard that starts a transaction automatically|
/// |auto guard = TransactionGuard(sqlConnector);|
/// |if (!guard.isValid()) { /* handle error */ }|
/// ||
/// |// ... perform database operations ...|
/// ||
/// |// Commit the transaction if all operations succeeded|
/// |if (!guard.finalize(true)) { /* handle commit error */ }|
///
/// @note If the guard goes out of scope without calling finalize(), the transaction
///       is automatically rolled back, preventing leaving the database in an
///       inconsistent state.
///
/// @thread_safety Not thread-safe. Each thread should use its own TransactionGuard
///                instance with a dedicated or properly synchronized SQLConnector.
class TransactionGuard
{
private:
    /// @brief Reference to the underlying SQL connector used for transaction control.
    Mantids30::Database::SQLConnector &m_sqlConnector;

    /// @brief Indicates whether a transaction was successfully started.
    bool m_transactionActive = false;

    /// @brief Indicates whether the transaction has been explicitly finalized (committed or rolled back).
    bool m_finalized = false;

public:
    /// @brief Constructs a TransactionGuard and starts a new database transaction.
    ///
    /// A transaction is immediately started on the provided SQLConnector. If the
    /// underlying beginTransaction() call fails, the guard is marked as invalid
    /// and subsequent operations will be no-ops.
    ///
    /// @param connector Reference to the SQL connector on which to start the transaction.
    explicit TransactionGuard(Mantids30::Database::SQLConnector &connector);

    /// @brief Explicitly finalizes the transaction by committing or rolling it back.
    ///
    /// If @p success is true, the transaction is committed; otherwise it is rolled back.
    /// This method may only be called once; subsequent calls return false without
    /// performing any action.
    ///
    /// @param success True to commit the transaction, false to roll it back.
    /// @return True if the transaction was successfully committed. Returns false if:
    ///   - The transaction is not active (never started successfully).
    ///   - The transaction was already finalized.
    ///   - A rollback was requested (always returns false in that case).
    ///   - The commit operation failed.
    [[nodiscard]] bool finalize(bool success);

    /// @brief Destructor that automatically rolls back unfinalized transactions.
    ///
    /// If the transaction is still active and has not been explicitly finalized,
    /// this destructor performs an automatic rollback to ensure the database is
    /// left in a consistent state.
    ~TransactionGuard();

    /// @brief Checks whether the transaction was successfully started and is still active.
    ///
    /// @return True if a transaction is active, false otherwise.
    [[nodiscard]] bool isValid() const { return m_transactionActive; }
};
} // namespace Mantids30::Database
