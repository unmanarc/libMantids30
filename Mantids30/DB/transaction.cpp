#include "transaction.h"

using namespace Mantids30::Database;

Transaction::Transaction(Mantids30::Database::SQLConnector &connector)
    : m_sqlConnector(connector)
    , m_transactionActive(connector.beginTransaction())
{}

bool Transaction::finalize(bool success)
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

Transaction::~Transaction()
{
    if (m_transactionActive && !m_finalized)
    {
        m_sqlConnector.rollbackTransaction();
    }
}
