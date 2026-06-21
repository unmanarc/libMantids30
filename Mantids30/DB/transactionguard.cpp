#include "transactionguard.h"


TransactionGuard::TransactionGuard(Mantids30::Database::SQLConnector &connector)
    : m_sqlConnector(connector)
    , m_transactionActive(connector.beginTransaction())
{}

bool TransactionGuard::finalize(bool success)
{
    if (!m_transactionActive || m_finalized)
    {
        return false;
    }
    m_finalized = true;

    if (success)
    {
        return m_sqlConnector.commitTransaction();
    }
    else
    {
        m_sqlConnector.rollbackTransaction();
        return false;
    }
}

TransactionGuard::~TransactionGuard()
{
    if (m_transactionActive && !m_finalized)
    {
        m_sqlConnector.rollbackTransaction();
    }
}
