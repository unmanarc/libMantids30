#include "socket_tls.h"

#include "socket_stream.h"

#include <memory>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

#include <stdexcept>
#include <system_error>
#include <string>

#include <openssl/dh.h>
#include <openssl/ssl.h>
#include <openssl/stack.h>
#include <openssl/x509_vfy.h>

#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#ifndef _WIN32
#include <arpa/inet.h>
#else
#include <ws2tcpip.h>
#endif

#ifdef _WIN32
#include <Mantids30/Core/Memory/w32compat.h>
#include <openssl/safestack.h>
#endif

using namespace std;
using namespace Mantids30::Network::Sockets;

Socket_TLS::Socket_TLS()
    : tlsKeys(&m_isServer)
{
#ifndef WIN32
    // Ignore sigpipes in this thread (eg. SSL_Write on closed socket):
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);
#endif

    setCertValidation(X509ValidationOption::VALIDATE);
}

Socket_TLS::~Socket_TLS()
{
    // shutdown the TLS socket:
    _shutdown(SHUT_RDWR);

    if (m_sslHandler)
    {
        SSL_free(m_sslHandler);
        m_sslHandler = nullptr;
    }
    if (m_sslContext)
    {
        SSL_CTX_free(m_sslContext);
        m_sslContext = nullptr;
    }
}

void Socket_TLS::prepareTLS()
{
    // Register the error strings for libcrypto & libssl
    SSL_load_error_strings();
    ERR_load_crypto_strings();
    // Register the available ciphers and digests
    SSL_library_init();

#ifndef _WIN32
    sigset_t sigPipeSet;
    sigemptyset(&sigPipeSet);
    sigaddset(&sigPipeSet, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &sigPipeSet, nullptr);
#endif
}

void Socket_TLS::setTLSParent(Socket_TLS *parent)
{
    m_tlsParentConnection = parent;
}

bool Socket_TLS::postConnectSubInitialization()
{
    if (m_sslHandler != nullptr)
    {
        return false; // already connected (don't connect again)
    }

    m_isServer = false;

    if (!createTLSContext())
    {
        return false;
    }

    if (!(m_sslHandler = SSL_new(m_sslContext)))
    {
        m_sslErrorList.emplace_back("SSL_new failed.");
        return false;
    }

    // If there is any configured PSK, put the key in the static list here...
    bool usingPSK = tlsKeys.linkPSKWithTLSHandle(m_sslHandler);

    // Initialize TLS client certificates and keys
    if (!tlsKeys.initTLSKeys(m_sslContext, m_sslHandler, &m_sslErrorList))
    {
        parseErrors();
        return false;
    }

    if (!(tlsKeys.getCAPath().empty()) || tlsKeys.getUseSystemCertificates())
    {
        SSL_set_verify(m_sslHandler, m_certValidationOptions == X509ValidationOption::NOVALIDATE ? SSL_VERIFY_NONE : SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT , nullptr);
    }
    else if (!usingPSK && m_certValidationOptions != X509ValidationOption::NOVALIDATE)
    {
        m_sslErrorList.emplace_back("TLS certificate validation requires a CA or system trust.");
        return false;
    }
    else
    {
        SSL_set_verify(m_sslHandler, SSL_VERIFY_NONE, nullptr);
    }

    // Set hostname for SNI extension
    if (tlsKeys.getUseSystemCertificates() || tlsKeys.getValidateServerHostname())
    {
        // Using system certificates...
        if (!SSL_set_tlsext_host_name(m_sslHandler, m_remoteServerHostname.c_str()))
        {
            m_sslErrorList.emplace_back("Unable to set TLS extension host name.");
            return false;
        }
    }

    if (!usingPSK && tlsKeys.getValidateServerHostname())
    {
        unsigned char address[sizeof(in6_addr)];
        X509_VERIFY_PARAM *parameters = SSL_get0_param(m_sslHandler);
        const bool isAddress = inet_pton(AF_INET, m_remoteServerHostname.c_str(), address) == 1 ||
                               inet_pton(AF_INET6, m_remoteServerHostname.c_str(), address) == 1;
        const int configured = isAddress ? X509_VERIFY_PARAM_set1_ip_asc(parameters, m_remoteServerHostname.c_str()) :
                                          X509_VERIFY_PARAM_set1_host(parameters, m_remoteServerHostname.c_str(), 0);
        if (m_remoteServerHostname.empty() || configured != 1)
        {
            m_sslErrorList.emplace_back("Unable to configure TLS hostname verification.");
            return false;
        }
    }

    if (SSL_set_fd(m_sslHandler, m_sockFD) != 1)
    {
        m_sslErrorList.emplace_back("SSL_set_fd failed.");
        return false;
    }

    if (SSL_get_error(m_sslHandler, SSL_connect(m_sslHandler)) != SSL_ERROR_NONE)
    {
        parseErrors();
        return false;
    }

    if (m_certValidationOptions != X509ValidationOption::NOVALIDATE)
    {
        // Using PKI, need to validate the certificate.
        // connected+validated!
        return validateTLSConnection(usingPSK) || m_certValidationOptions == X509ValidationOption::CHECKANDPASS;
    }
    // no validate here...
    else
    {
        return true;
    }
}

bool Socket_TLS::postAcceptSubInitialization()
{
    if (m_sslHandler != nullptr)
    {
        return false; // already connected (don't connect again)
    }

    m_isServer = true;

    if (!createTLSContext())
    {
        return false;
    }

    // ssl empty, create a new one.
    if (!(m_sslHandler = SSL_new(m_sslContext)))
    {
        m_sslErrorList.emplace_back("SSL_new failed.");
        return false;
    }

    // If the parent have any PSK key, pass everything to the current socket_tls.
    *(tlsKeys.getPSKServerWallet()) = *(m_tlsParentConnection->tlsKeys.getPSKServerWallet());

    bool usingPSK = isUsingPSK();
    // If there is any configured PSK, link the key in the static list...
    if (usingPSK)
    {
        tlsKeys.linkPSKWithTLSHandle(m_sslHandler);
    }

    // in server mode, use the parent keys...
    if (!m_tlsParentConnection->tlsKeys.initTLSKeys(m_sslContext, m_sslHandler, &m_sslErrorList))
    {
        parseErrors();
        return false;
    }

    if (!m_tlsParentConnection->tlsKeys.getCAPath().empty() || m_tlsParentConnection->tlsKeys.getUseSystemCertificates())
    {
        SSL_set_verify(m_sslHandler, m_certValidationOptions == X509ValidationOption::NOVALIDATE ? SSL_VERIFY_NONE : (SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT) , nullptr);
    }
    else
    {
        // If there is no CA...
        m_certValidationOptions = X509ValidationOption::NOVALIDATE;
        //SSL_set_verify(sslh, X509ValidationOption::NOVALIDATE, nullptr);
    }

    if (SSL_set_fd(m_sslHandler, m_sockFD) != 1)
    {
        m_sslErrorList.emplace_back("SSL_set_fd failed.");
        return false;
    }

    int err;
    if ((err = SSL_accept(m_sslHandler)) != 1)
    {
        parseErrors();
        return false;
    }

    if (m_certValidationOptions != X509ValidationOption::NOVALIDATE)
    {
        // Using PKI, need to validate the certificate.
        // connected+validated!
        return validateTLSConnection(usingPSK) || m_certValidationOptions == X509ValidationOption::CHECKANDPASS;
    }
    // no validate here...
    else
    {
        return true;
    }
}

SSL_CTX *Socket_TLS::createServerSSLContext()
{
#if TLS_MAX_VERSION == TLS1_VERSION
    return SSL_CTX_new(TLSv1_server_method());
#elif TLS_MAX_VERSION == TLS1_1_VERSION
    return SSL_CTX_new(TLSv1_1_server_method());
#elif TLS_MAX_VERSION == TLS1_2_VERSION
    return SSL_CTX_new(TLSv1_2_server_method());
#elif TLS_MAX_VERSION >= TLS1_3_VERSION
    return SSL_CTX_new(TLS_server_method());
#else
    return nullptr;
#endif
}

SSL_CTX *Socket_TLS::createClientSSLContext()
{
#if TLS_MAX_VERSION == TLS1_VERSION
    return SSL_CTX_new(TLSv1_client_method());
#elif TLS_MAX_VERSION == TLS1_1_VERSION
    return SSL_CTX_new(TLSv1_1_client_method());
#elif TLS_MAX_VERSION == TLS1_2_VERSION
    return SSL_CTX_new(TLSv1_2_client_method());
#elif TLS_MAX_VERSION >= TLS1_3_VERSION
    return SSL_CTX_new(TLS_client_method());
#else
    return nullptr;
#endif
}

bool Socket_TLS::isServer() const
{
    return m_isServer;
}

bool Socket_TLS::isUsingPSK() const
{
    if (m_isServer)
    {
        return m_tlsParentConnection->tlsKeys.getPSKServerWallet()->isUsingPSK;
    }
    else
    {
        return tlsKeys.getPSKClientValue()->isUsingPSK;
    }
}

bool Socket_TLS::createTLSContext()
{
    // create new SSL Context.

    if (m_sslContext)
    {
        throw std::runtime_error("Can't reuse the TLS socket. Create a new one.");
    }

    if (m_isServer)
    {
        m_sslContext = createServerSSLContext();
        if (!m_sslContext)
        {
            m_sslErrorList.emplace_back("TLS_server_method() Failed.");
            return false;
        }
    }
    else
    {
        m_sslContext = createClientSSLContext();
        if (!m_sslContext)
        {
            m_sslErrorList.emplace_back("TLS_client_method() Failed.");
            return false;
        }
    }

    return true;
}

void Socket_TLS::parseErrors()
{
    char buf[512];
    unsigned long int err;
    while ((err = ERR_get_error()) != 0)
    {
        ERR_error_string_n(err, buf, sizeof(buf));
        m_sslErrorList.emplace_back(buf);
    }
}

bool Socket_TLS::validateTLSConnection(const bool &usingPSK)
{
    if (!m_sslHandler)
    {
        return false;
    }

    bool bValid = false;

    if (!usingPSK)
    {
        X509 *cert;
        cert = SSL_get_peer_certificate(m_sslHandler);
        if (cert != nullptr)
        {
            long res = SSL_get_verify_result(m_sslHandler);
            if (res == X509_V_OK)
            {
                bValid = true;
            }
            else
            {
                m_sslErrorList.emplace_back("Peer TLS/SSL Certificate Verification Error (" + std::to_string(res) + "): " + std::string(X509_verify_cert_error_string(res)));
            }
            X509_free(cert);
        }
        else
        {
            m_sslErrorList.emplace_back("Peer TLS/SSL Certificate does not exist.");
        }
    }
    else
    {
        bValid = true;
    }

    return bValid;
}

Socket_TLS::X509ValidationOption Socket_TLS::getCertValidation() const
{
    return m_certValidationOptions;
}

void Socket_TLS::setCertValidation(X509ValidationOption newCertValidation)
{
    m_certValidationOptions = newCertValidation;
}

string Socket_TLS::getTLSConnectionCipherName()
{
    if (!m_sslHandler)
    {
        return "";
    }
    return SSL_get_cipher_name(m_sslHandler);
}

std::list<std::string> Socket_TLS::getTLSErrorsAndClear()
{
    std::list<std::string> sslErrors2 = m_sslErrorList;
    m_sslErrorList.clear();
    return sslErrors2;
}

string Socket_TLS::getPeerName() const
{
    return getTLSPeerCN();
}

string Socket_TLS::getTLSPeerCN() const
{
    if (!m_sslHandler)
    {
        return "";
    }

    if (!isUsingPSK())
    {
        char certCNText[512];
        memset(certCNText, 0, sizeof(certCNText));

        X509 *cert = SSL_get_peer_certificate(m_sslHandler);
        if (cert)
        {
            X509_NAME *certName = X509_get_subject_name(cert);
            if (certName)
            {
                X509_NAME_get_text_by_NID(certName, NID_commonName, certCNText, 511);
            }
            X509_free(cert);
        }
        return certCNText;
    }
    else
    {
        // the remote host is the server... no configured id.
        if (!m_isServer)
        {
            return "server";
        }
        else
        {
            return tlsKeys.getPSKServerWallet()->connectedClientID;
        }
    }
}



int Socket_TLS::_shutdown(int mode)
{
    if (!isActive())
    {
        return -1;
    }
    if (mode != SHUT_RD && mode != SHUT_WR && mode != SHUT_RDWR)
    {
        return -1;
    }
    if (!m_sslHandler)
    {
        const int transportStatus = Socket::iShutdown(mode);
        if (isServer() || m_isInListenMode)
        {
            return transportStatus;
        }
        if (m_lastError.empty())
        {
            m_lastError = "TLS shutdown requested before SSL handle initialization.";
        }
        return -4;
    }
    int tlsStatus = 0;
    auto recordFailure = [this](const std::string &message)
    {
        m_sslErrorList.emplace_back(message);
        if (m_lastError.empty())
        {
            m_lastError = message;
        }
    };
    if (mode != SHUT_RD && !m_shutdownProtocolOnWrite && m_sslHandler && SSL_is_init_finished(m_sslHandler) &&
        !(SSL_get_shutdown(m_sslHandler) & SSL_SENT_SHUTDOWN))
    {
        // Send close_notify without waiting for the peer. Preserve diagnostics
        // and the incomplete (-2) versus failed (-3) TLS shutdown distinction.
        auto sendCloseNotify = [&]
        {
            parseErrors();
            int result = SSL_shutdown(m_sslHandler);
            if (result == 0)
            {
                // The descriptor is nonblocking: retain the original second
                // shutdown step when peer confirmation is already available.
                result = SSL_shutdown(m_sslHandler);
            }
            // Always drain the thread's OpenSSL error queue so a stale entry
            // never leaks into the next connection handled on this thread.
            parseErrors();
            const int systemError = getLastSocketError();
            if (result == 0)
            {
                // Expected for short-lived peers that do not complete the
                // bidirectional close. Keep the -2 status and the legacy
                // lastError diagnostic, but do not record it in the TLS error
                // list so it does not surface on the next connection.
                tlsStatus = -2;
                if (m_lastError.empty())
                {
                    m_lastError = "SSL_shutdown() incomplete: close_notify sent; peer confirmation is not yet available.";
                }
            }
            else if (result < 0)
            {
                const int sslError = SSL_get_error(m_sslHandler, result);
                if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE)
                {
                    tlsStatus = -2;
                    if (m_lastError.empty())
                    {
                        m_lastError = "SSL_shutdown() incomplete: SSL error #" + std::to_string(sslError) +
                                      "; additional I/O would be required.";
                    }
                }
                else
                {
                    tlsStatus = -3;
                    recordFailure("SSL_shutdown() failed: SSL error #" + std::to_string(sslError) + ", system error #" +
                                  std::to_string(systemError) + ": " + std::system_category().message(systemError));
                }
            }
        };
#ifndef _WIN32
        const int flags = fcntl(m_sockFD, F_GETFL, 0);
        if (flags >= 0 && fcntl(m_sockFD, F_SETFL, flags | O_NONBLOCK) == 0)
        {
            sendCloseNotify();
            if (fcntl(m_sockFD, F_SETFL, flags) < 0)
            {
                const int error = errno;
                tlsStatus = -3;
                recordFailure("TLS shutdown: restoring descriptor flags failed with error #" + std::to_string(error) + ": " + std::system_category().message(error));
            }
        }
        else
        {
            const int error = errno;
            tlsStatus = -3;
            recordFailure("TLS shutdown: setting nonblocking mode failed with error #" + std::to_string(error) + ": " + std::system_category().message(error));
        }
#else
        if (setBlockingMode(false))
        {
            sendCloseNotify();
            if (!setBlockingMode(true))
            {
                tlsStatus = -3;
                recordFailure(m_lastError);
            }
        }
        else
        {
            tlsStatus = -3;
            recordFailure(m_lastError);
        }
#endif
    }
    const int transportStatus = Socket::iShutdown(mode);
    return transportStatus != 0 ? transportStatus : tlsStatus;
}

int Socket_TLS::iShutdown(int mode)
{
    return _shutdown(mode);
}

bool Socket_TLS::isSecure()
{
    return true;
}

void Socket_TLS::setServerMode(bool value)
{
    m_isServer = value;
}

Socket_TLS::sCipherBits Socket_TLS::getTLSConnectionCipherBits()
{
    sCipherBits cb;
    if (!m_sslHandler)
    {
        return cb;
    }
    cb.asymmetricBits = SSL_get_cipher_bits(m_sslHandler, &cb.symmetricBits);
    return cb;
}

string Socket_TLS::getTLSConnectionProtocolVersion()
{
    if (!m_sslHandler)
    {
        return "";
    }
    return SSL_get_version(m_sslHandler);
}

std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> Socket_TLS::acceptConnection()
{
    char remotePair[INET6_ADDRSTRLEN];

    m_isServer = true;

    std::shared_ptr<Socket_Stream> acceptedTCPSock = Socket_TCP::acceptConnection();

    if (!acceptedTCPSock)
    {
        return nullptr;
    }

    std::shared_ptr<Socket_TLS> acceptedTLSSock = std::make_shared<Socket_TLS>(); // Convert to this thing...

    // Set current retrieved socket info.
    acceptedTCPSock->getRemotePair(remotePair);
    acceptedTLSSock->setRemotePair(remotePair);
    acceptedTLSSock->setRemotePort(acceptedTCPSock->getRemotePort());

    // Inehrit certificates...
    acceptedTLSSock->setTLSParent(this);

    // Set contexts and modes...
    acceptedTLSSock->setServerMode(m_isServer);

    // now we should copy the file descriptor:
    acceptedTLSSock->setUseIPv6(acceptedTCPSock->getUseIPv6());
    acceptedTLSSock->setSocketFD(acceptedTCPSock->adquireSocketFD());
    acceptedTLSSock->setReadTimeout(m_readTimeout);
    acceptedTLSSock->setWriteTimeout(m_writeTimeout);

    // After this, the postInitialization will be called by the acceptor thread.
    return acceptedTLSSock;
}

ssize_t Socket_TLS::partialRead(void *data, const size_t &datalen)
{
    std::unique_lock<std::mutex> lock(mutexRead);

    return iPartialRead(data, datalen);
}

ssize_t Socket_TLS::partialWrite(const void *data, const size_t &datalen)
{
    std::unique_lock<std::mutex> lock(mutexWrite);

    return iPartialWrite(data, datalen);
}

ssize_t Socket_TLS::iPartialRead(void *data, const size_t &datalen, int ttl)
{
    if (!m_sslHandler)
    {
        m_lastError = "SSL handle is null";
        return -1;
    }

    if (ttl == 0)
    {
        return -1;
    }

    if (datalen == 0)
    {
        return 0;
    }
    if (!data || datalen > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        return -1;
    }
    parseErrors();
    int readBytes = SSL_read(m_sslHandler, data, static_cast<int>(datalen));

    if (readBytes > 0)
    {
        // Debug print for read data
        if ((debugOptions & Socket::DebugOptions::PRINT_READ_HEX) || (debugOptions & Socket::DebugOptions::PRINT_READ_PLAIN))
        {
            if (debugOptions & Socket::DebugOptions::PRINT_READ_HEX)
            {
                fprintf(debugFP, "<<< [TLS READ] Read %d bytes\n", readBytes);
                fflush(debugFP);
                // Print hex dump using BIO_dump_fp
                BIO *bio = BIO_new(BIO_s_mem());
                if (bio)
                {
                    BIO_dump_fp(debugFP, static_cast<const char *>(data), readBytes);
                    fflush(debugFP);
                    BIO_free(bio);
                }
            }
            else
            {
                std::string datax;
                datax.append(static_cast<const char *>(data), readBytes);
                fprintf(debugFP, "%s", datax.c_str());
                fflush(debugFP);
            }
        }

        m_lastError.clear();
        return readBytes;
    }
    else if (SSL_get_error(m_sslHandler, readBytes) == SSL_ERROR_ZERO_RETURN)
    {
        // The connection has been closed.
        if (debugOptions & Socket::DebugOptions::PRINT_CLOSE)
        {
            fprintf(debugFP, "+++ [TLS CLOSE] Connection closed by peer\n");
            fflush(debugFP);
        }

        m_lastError = "Connection closed by peer";
        return 0;
    }

    // Error mgmt:
    int sslError = SSL_get_error(m_sslHandler, readBytes);
    switch (sslError)
    {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        // Try Again after 10ms...

        if (debugOptions & Socket::DebugOptions::PRINT_ERRORS)
        {
            fprintf(debugFP, "--- [TLS READ] SSL_WANT_READ/WRITE during read, retrying...\n");
            fflush(debugFP);
        }

        usleep(10000);
        return iPartialRead(data, datalen, ttl - 1);
    case SSL_ERROR_SYSCALL:
    {
        const int systemError = getLastSocketError();
        const std::string errorBuffer = std::system_category().message(systemError);
        m_lastError = std::string("System call error: ") + errorBuffer;

        if (debugOptions & Socket::DebugOptions::PRINT_ERRORS)
        {
            fprintf(debugFP, "--- [TLS ERROR] System call error during read: %s\n", errorBuffer.c_str());
            fflush(debugFP);
        }

        break;
    }
    case SSL_ERROR_ZERO_RETURN:
    case SSL_ERROR_SSL:
        // SSL Specific error.
        {
            parseErrors();
            m_lastError = std::string("SSL Layer Error");

            if (debugOptions & Socket::DebugOptions::PRINT_ERRORS)
            {
                fprintf(debugFP, "--- [TLS ERROR] SSL Layer Error during read\n");
                fflush(debugFP);
            }
            break;
        }
    default:
        m_lastError = "Unknown SSL error occurred";

        if (debugOptions & Socket::DebugOptions::PRINT_ERRORS)
        {
            fprintf(debugFP, "--- [TLS ERROR] Unknown SSL error occurred during read\n");
            fflush(debugFP);
        }
    }

    // In case of error, close the connection.
    Socket_TCP::iShutdown();

    return -1;
}

ssize_t Socket_TLS::iPartialWrite(const void *data, const size_t &datalen, int ttl)
{
    if (!m_sslHandler)
    {
        return -1;
    }

    if (ttl == 0)
    {
        return -1;
    }

    if (datalen > static_cast<uint64_t>(std::numeric_limits<int>::max()))
    {
        throw std::runtime_error("Data size exceeds the maximum allowed for partial write.");
    }

    int chunkSize = static_cast<int>(datalen);

    // Debug: Print write data if enabled
    if (debugOptions & Socket::DebugOptions::PRINT_WRITE_HEX)
    {
        fprintf(debugFP, ">>> [TLS_WRITE] Writing %d bytes\n", chunkSize);
        fflush(debugFP);
    }

    if (datalen == 0)
    {
        return 0;
    }
    if (!data)
    {
        return -1;
    }
    parseErrors();
    int sentBytes = SSL_write(m_sslHandler, data, chunkSize);

    if (sentBytes > 0)
    {
        m_lastError.clear();

        // Debug: Print sent bytes if enabled
        if ((debugOptions & Socket::DebugOptions::PRINT_WRITE_HEX) || (debugOptions & Socket::DebugOptions::PRINT_WRITE_PLAIN))
        {
            if (debugOptions & Socket::DebugOptions::PRINT_WRITE_HEX)
            {
                fprintf(debugFP, ">>> [TLS_WRITE] Wrote %d bytes\n", sentBytes);
                BIO_dump_fp(debugFP, static_cast<const char *>(data), sentBytes);
                fflush(debugFP);
            }
            else
            {
                std::string datax;
                datax.append(static_cast<const char *>(data), sentBytes);
                fprintf(debugFP, "%s", datax.c_str());
                fflush(debugFP);
            }
        }

        return sentBytes;
    }
    else if (sentBytes == 0)
    {
        // Closed.
        m_lastError = "Connection closed";
        // Debug: Print close event if enabled
        if (debugOptions & Socket::DebugOptions::PRINT_CLOSE)
        {
            fprintf(debugFP, "+++ [TLS_CLOSE] Connection closed by peer\n");
            fflush(debugFP);
        }

        return sentBytes;
    }
    else
    {
        int sslErr = SSL_get_error(m_sslHandler, sentBytes);
        switch (sslErr)
        {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            // Wait 10ms... and try again...
            usleep(10000);
            // Debug: Print retry info if enabled
            if (debugOptions & Socket::DebugOptions::PRINT_WRITE_HEX)
            {
                fprintf(debugFP, "--- [TLS_WRITE] SSL_WANT_RETRY, retrying (ttl=%d)\n", ttl);
                fflush(debugFP);
            }

            return iPartialWrite(data, datalen, ttl - 1);
        case SSL_ERROR_SYSCALL:
        {
            const int systemError = getLastSocketError();
            const std::string errorBuffer = std::system_category().message(systemError);
            m_lastError = std::string("System call error: ") + errorBuffer;
            parseErrors();
            Socket_TCP::iShutdown();

            // Debug: Print syscall error if enabled
            if (debugOptions & Socket::DebugOptions::PRINT_WRITE_HEX)
            {
                fprintf(debugFP, "--- [TLS_WRITE_ERROR] System call error: %s\n", errorBuffer.c_str());
                fflush(debugFP);
            }

            return -1;
        }
        case SSL_ERROR_ZERO_RETURN:
        case SSL_ERROR_SSL:
        default:
            // Other SSL errors
            m_lastError = std::string("SSL Layer Error");
            parseErrors();
            Socket_TCP::iShutdown();

            // Debug: Print SSL error if enabled
            if (debugOptions & Socket::DebugOptions::PRINT_WRITE_HEX)
            {
                fprintf(debugFP, "--- [TLS_WRITE_ERROR] SSL error occurred\n");
                fflush(debugFP);
            }
            return -1;
        }
    }
}
