#include "jwt.h"

using namespace Mantids30::DataFormat;

JWT::Revocation::Revocation()
{
    m_stopGarbageCollector = false;
    m_garbageCollectorInterval = std::chrono::seconds(10);
    m_garbageCollectorThread = std::thread(&Revocation::garbageCollector, this);
}

JWT::Revocation::~Revocation()
{
    m_stopGarbageCollector = true;
    m_garbageCollectorCondition.notify_one();
    if (m_garbageCollectorThread.joinable()) {
        m_garbageCollectorThread.join();
    }
}

void JWT::Revocation::addToRevocationList(const std::string &signature, time_t expirationTime)
{
    boost::unique_lock<boost::shared_mutex> writeLock(m_revokedTokensMutex);
    m_expirationSignatures.insert({expirationTime, signature}); 
    m_revokedTokens.insert(signature);
}

bool JWT::Revocation::isSignatureRevoked(const std::string &signature)
{
    boost::shared_lock<boost::shared_mutex> readLock(m_revokedTokensMutex);

    // TODO: can you modify a signature to ...
    return m_revokedTokens.find(signature) != m_revokedTokens.end();
}

void JWT::Revocation::removeExpiredTokensFromRevocationList()
{
    boost::unique_lock<boost::shared_mutex> writeLock(m_revokedTokensMutex);

    time_t now = std::time(nullptr);
    std::multimap<time_t, std::string>::iterator upperBound = m_expirationSignatures.upper_bound(now);
    for (std::multimap<time_t, std::string>::iterator it = m_expirationSignatures.begin(); it != upperBound; ++it)
    {
        m_revokedTokens.erase(it->second);
    }
    
    m_expirationSignatures.erase(m_expirationSignatures.begin(), upperBound);
}

std::chrono::seconds JWT::Revocation::garbageCollectorInterval() const
{
    return m_garbageCollectorInterval;
}

void JWT::Revocation::setGarbageCollectorInterval(const std::chrono::seconds &newGarbageCollectorInterval)
{
    m_garbageCollectorInterval = newGarbageCollectorInterval;
}

void JWT::Revocation::garbageCollector()
{
    std::unique_lock<std::mutex> lock(m_garbageCollectorMutex);
    while (!m_stopGarbageCollector)
    {
        m_garbageCollectorCondition.wait_for(lock, m_garbageCollectorInterval);
        if (!m_stopGarbageCollector)
        {
            removeExpiredTokensFromRevocationList();
        }
    }
}
