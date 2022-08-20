#include "socket_tls.h"
#include <stdexcept>

using namespace std;
using namespace Mantids::Network::Sockets;

std::map<void *,Socket_TLS::TLSKeyParameters::PSKClientValue *> Socket_TLS::TLSKeyParameters::PSKStaticHdlr::cliPSKBySSLH;
std::map<void *,Socket_TLS::TLSKeyParameters::PSKServerWallet *> Socket_TLS::TLSKeyParameters::PSKStaticHdlr::svrPSKBySSLH;
std::mutex Socket_TLS::TLSKeyParameters::PSKStaticHdlr::mCLIPSKBySSLH;
std::mutex Socket_TLS::TLSKeyParameters::PSKStaticHdlr::mSVRPSKBySSLH;

Socket_TLS::TLSKeyParameters::PSKStaticHdlr::PSKStaticHdlr(PSKClientValue *pskClientValues, PSKServerWallet *pskServerValues)
{
    this->pskClientValues = pskClientValues;
    this->pskServerValues = pskServerValues;
    sslhForPSK = nullptr;
}



Socket_TLS::TLSKeyParameters::PSKStaticHdlr::~PSKStaticHdlr()
{
    if (1)
    {
        std::unique_lock<std::mutex> lock(mSVRPSKBySSLH);
        if (sslhForPSK)
        {
            if (!svrPSKBySSLH.empty() && svrPSKBySSLH.find( sslhForPSK ) != svrPSKBySSLH.end())
                svrPSKBySSLH.erase(sslhForPSK);
        }
    }

    if (1)
    {
        std::unique_lock<std::mutex> lock(mCLIPSKBySSLH);
        if (sslhForPSK)
        {
            if (!cliPSKBySSLH.empty() && cliPSKBySSLH.find( sslhForPSK ) != cliPSKBySSLH.end())
                cliPSKBySSLH.erase(sslhForPSK);
        }
    }
}

Socket_TLS::TLSKeyParameters::PSKServerWallet *Socket_TLS::TLSKeyParameters::PSKStaticHdlr::getServerWallet(void *sslh)
{
    std::unique_lock<std::mutex> lock(mSVRPSKBySSLH);
    if (svrPSKBySSLH.find(sslh) != svrPSKBySSLH.end())
        return svrPSKBySSLH[sslh];
    return nullptr;
}

Socket_TLS::TLSKeyParameters::PSKClientValue *Socket_TLS::TLSKeyParameters::PSKStaticHdlr::getClientValue(void *sslh)
{
    std::unique_lock<std::mutex> lock(mCLIPSKBySSLH);
    if (cliPSKBySSLH.find(sslh) != cliPSKBySSLH.end())
        return cliPSKBySSLH[sslh];
    return nullptr;
}

bool Socket_TLS::TLSKeyParameters::PSKStaticHdlr::setSSLHandler(SSL *sslh)
{
    if (sslhForPSK)
        throw std::runtime_error("Can't reuse the PSK and Socket. Create a new one (1).");

    sslhForPSK = sslh;

    if (pskServerValues->usingPSK)
    {
        std::unique_lock<std::mutex> lock(mSVRPSKBySSLH);
        if (svrPSKBySSLH.find( sslhForPSK ) == svrPSKBySSLH.end())
            svrPSKBySSLH[sslhForPSK] = pskServerValues;
    }

    if (pskClientValues->usingPSK)
    {
        std::unique_lock<std::mutex> lock(mCLIPSKBySSLH);
        if (cliPSKBySSLH.find( sslhForPSK ) == cliPSKBySSLH.end())
            cliPSKBySSLH[sslhForPSK] = pskClientValues;
    }

    return (pskClientValues->usingPSK || pskServerValues->usingPSK);
}
