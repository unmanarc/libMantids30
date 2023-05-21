#include "socket_tls.h"
#include <stdexcept>

using namespace std;
using namespace Mantids29::Network::Sockets;

std::map<void *,Socket_TLS::TLSKeyParameters::PSKClientValue *> Socket_TLS::TLSKeyParameters::PSKStaticHdlr::m_clientPSKBySSLHandlerMap;
std::map<void *,Socket_TLS::TLSKeyParameters::PSKServerWallet *> Socket_TLS::TLSKeyParameters::PSKStaticHdlr::m_serverPSKBySSLHandlerMap;
std::mutex Socket_TLS::TLSKeyParameters::PSKStaticHdlr::m_clientPSKBySSLHandlerMapMutex;
std::mutex Socket_TLS::TLSKeyParameters::PSKStaticHdlr::m_serverPSKBySSLHandlerMapMutex;

Socket_TLS::TLSKeyParameters::PSKStaticHdlr::PSKStaticHdlr(PSKClientValue *pskClientValues, PSKServerWallet *pskServerValues)
{
    this->m_pskClientValues = pskClientValues;
    this->m_pskServerValues = pskServerValues;
    m_sslHandlerForPSK = nullptr;
}

Socket_TLS::TLSKeyParameters::PSKStaticHdlr::~PSKStaticHdlr()
{
    if (1)
    {
        std::unique_lock<std::mutex> lock(m_serverPSKBySSLHandlerMapMutex);
        if (m_sslHandlerForPSK)
        {
            if (!m_serverPSKBySSLHandlerMap.empty() && m_serverPSKBySSLHandlerMap.find( m_sslHandlerForPSK ) != m_serverPSKBySSLHandlerMap.end())
                m_serverPSKBySSLHandlerMap.erase(m_sslHandlerForPSK);
        }
    }

    if (1)
    {
        std::unique_lock<std::mutex> lock(m_clientPSKBySSLHandlerMapMutex);
        if (m_sslHandlerForPSK)
        {
            if (!m_clientPSKBySSLHandlerMap.empty() && m_clientPSKBySSLHandlerMap.find( m_sslHandlerForPSK ) != m_clientPSKBySSLHandlerMap.end())
                m_clientPSKBySSLHandlerMap.erase(m_sslHandlerForPSK);
        }
    }
}

Socket_TLS::TLSKeyParameters::PSKServerWallet *Socket_TLS::TLSKeyParameters::PSKStaticHdlr::getServerWallet(void *sslh)
{
    std::unique_lock<std::mutex> lock(m_serverPSKBySSLHandlerMapMutex);
    if (m_serverPSKBySSLHandlerMap.find(sslh) != m_serverPSKBySSLHandlerMap.end())
        return m_serverPSKBySSLHandlerMap[sslh];
    return nullptr;
}

Socket_TLS::TLSKeyParameters::PSKClientValue *Socket_TLS::TLSKeyParameters::PSKStaticHdlr::getClientValue(void *sslh)
{
    std::unique_lock<std::mutex> lock(m_clientPSKBySSLHandlerMapMutex);
    if (m_clientPSKBySSLHandlerMap.find(sslh) != m_clientPSKBySSLHandlerMap.end())
        return m_clientPSKBySSLHandlerMap[sslh];
    return nullptr;
}

bool Socket_TLS::TLSKeyParameters::PSKStaticHdlr::setSSLHandler(SSL *sslh)
{
    if (m_sslHandlerForPSK)
        throw std::runtime_error("Can't reuse the PSK and Socket. Create a new one (1).");
    
    m_sslHandlerForPSK = sslh;
    
    if (m_pskServerValues->m_isUsingPSK)
    {
        std::unique_lock<std::mutex> lock(m_serverPSKBySSLHandlerMapMutex);
        if (m_serverPSKBySSLHandlerMap.find( m_sslHandlerForPSK ) == m_serverPSKBySSLHandlerMap.end())
            m_serverPSKBySSLHandlerMap[m_sslHandlerForPSK] = m_pskServerValues;
    }
    
    if (m_pskClientValues->m_isUsingPSK)
    {
        std::unique_lock<std::mutex> lock(m_clientPSKBySSLHandlerMapMutex);
        if (m_clientPSKBySSLHandlerMap.find( m_sslHandlerForPSK ) == m_clientPSKBySSLHandlerMap.end())
            m_clientPSKBySSLHandlerMap[m_sslHandlerForPSK] = m_pskClientValues;
    }
    
    return (m_pskClientValues->m_isUsingPSK || m_pskServerValues->m_isUsingPSK);
}
