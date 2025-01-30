#include "socket_tls.h"

#include "socket_stream_base.h"


#include <memory>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#include <limits>
#include <string>
#include <stdexcept>

#include <openssl/ssl.h>
#include <openssl/dh.h>
#include <openssl/x509_vfy.h>
#include <openssl/stack.h>

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>


#ifdef _WIN32
#include <openssl/safestack.h>
#include <Mantids30/Memory/w32compat.h>
#endif

using namespace std;
using namespace Mantids30::Network::Sockets;

Socket_TLS::Socket_TLS() : m_keys(&m_isServer)
{
#ifndef WIN32
    // Ignore sigpipes in this thread (eg. SSL_Write on closed socket):
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
#endif

    setCertValidation(CERT_X509_VALIDATE);
    m_tlsParentConnection = nullptr;
    m_isServer = false;
    m_sslHandler = nullptr;
    m_sslContext = nullptr;
}

Socket_TLS::~Socket_TLS()
{
    if (m_sslHandler)
        SSL_free (m_sslHandler);
    if (m_sslContext)
        SSL_CTX_free(m_sslContext);
}

void Socket_TLS::prepareTLS()
{
    // Register the error strings for libcrypto & libssl
    SSL_load_error_strings ();
    ERR_load_crypto_strings();
    // Register the available ciphers and digests
    SSL_library_init ();

#ifndef _WIN32
    sigset_t sigPipeSet;
    sigemptyset(&sigPipeSet);
    sigaddset(&sigPipeSet, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &sigPipeSet, NULL);
#endif
}

void Socket_TLS::setTLSParent(Socket_TLS *parent)
{
    m_tlsParentConnection = parent;
}

bool Socket_TLS::postConnectSubInitialization()
{
    if (m_sslHandler!=nullptr)
        return false; // already connected (don't connect again)

    m_isServer = false;

    if (!createTLSContext())
    {
        return false;
    }

    if (!(m_sslHandler = SSL_new(m_sslContext)))
    {
        m_sslErrorList.push_back("SSL_new failed.");
        return false;
    }

    // If there is any configured PSK, put the key in the static list here...
    bool usingPSK = m_keys.linkPSKWithTLSHandle(m_sslHandler);

    // Initialize TLS client certificates and keys
    if (!m_keys.initTLSKeys(m_sslContext,m_sslHandler, &m_sslErrorList))
    {
        parseErrors();
        return false;
    }
    
    if ( !(m_keys.getCAPath().empty()) || m_keys.getUseSystemCertificates() )
        SSL_set_verify(m_sslHandler, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | (m_certValidationOptions==CERT_X509_NOVALIDATE?SSL_VERIFY_NONE:0) , nullptr);
    else
    {
        // If there is no CA...
        m_certValidationOptions=CERT_X509_NOVALIDATE;
    }

    // Set hostname for SNI extension
    if (m_keys.getUseSystemCertificates() || m_keys.getValidateServerHostname())
    {
        // Using system certificates...
        if (!SSL_set_tlsext_host_name(m_sslHandler, m_remoteServerHostname.c_str()))
        {
            m_sslErrorList.push_back("Unable to set TLS extension host name.");
            return false;
        }
    }

    if (SSL_set_fd (m_sslHandler, m_sockFD) != 1)
    {
        m_sslErrorList.push_back("SSL_set_fd failed.");
        return false;
    }

    if ( SSL_get_error(m_sslHandler, SSL_connect (m_sslHandler)) != SSL_ERROR_NONE )
    {
        parseErrors();
        return false;
    }
    
    if ( m_certValidationOptions!=CERT_X509_NOVALIDATE )
    {
        // Using PKI, need to validate the certificate.
        // connected+validated!
        return validateTLSConnection(usingPSK) || m_certValidationOptions==CERT_X509_CHECKANDPASS;
    }
    // no validate here...
    else
        return true;
}

bool Socket_TLS::postAcceptSubInitialization()
{
    if (m_sslHandler!=nullptr)
        return false; // already connected (don't connect again)

    m_isServer = true;

    if (!createTLSContext())
    {
        return false;
    }

    // ssl empty, create a new one.
    if (!(m_sslHandler = SSL_new(m_sslContext)))
    {
        m_sslErrorList.push_back("SSL_new failed.");
        return false;
    }

    // If the parent have any PSK key, pass everything to the current socket_tls.
    *(m_keys.getPSKServerWallet()) = *(m_tlsParentConnection->m_keys.getPSKServerWallet());

    bool usingPSK = isUsingPSK();
    // If there is any configured PSK, link the key in the static list...
    if ( usingPSK )
    {
        m_keys.linkPSKWithTLSHandle(m_sslHandler);
    }

    // in server mode, use the parent keys...
    if (!m_tlsParentConnection->m_keys.initTLSKeys(m_sslContext,m_sslHandler, &m_sslErrorList))
    {
        parseErrors();
        return false;
    }
    
    if ( !m_tlsParentConnection->m_keys.getCAPath().empty() || m_tlsParentConnection->m_keys.getUseSystemCertificates() )
        SSL_set_verify(m_sslHandler, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | (m_certValidationOptions==CERT_X509_NOVALIDATE?SSL_VERIFY_NONE:0) , nullptr);
    else
    {
        // If there is no CA...
        m_certValidationOptions=CERT_X509_NOVALIDATE;
        //SSL_set_verify(sslh, CERT_X509_NOVALIDATE, nullptr);
    }

    if (SSL_set_fd (m_sslHandler, m_sockFD) != 1)
    {
        m_sslErrorList.push_back("SSL_set_fd failed.");
        return false;
    }

    int err;
    if ((err=SSL_accept(m_sslHandler)) != 1)
    {
        parseErrors();
        return false;
    }
    
    if ( m_certValidationOptions!=CERT_X509_NOVALIDATE )
    {
        // Using PKI, need to validate the certificate.
        // connected+validated!
        return validateTLSConnection(usingPSK) || m_certValidationOptions==CERT_X509_CHECKANDPASS;
    }
    // no validate here...
    else
        return true;
}

SSL_CTX *Socket_TLS::createServerSSLContext()
{
#if TLS_MAX_VERSION == TLS1_VERSION
    return SSL_CTX_new (TLSv1_server_method());
#elif TLS_MAX_VERSION == TLS1_1_VERSION
    return SSL_CTX_new (TLSv1_1_server_method());
#elif TLS_MAX_VERSION == TLS1_2_VERSION
    return SSL_CTX_new (TLSv1_2_server_method());
#elif TLS_MAX_VERSION >= TLS1_3_VERSION
    return SSL_CTX_new (TLS_server_method());
#endif
    return nullptr;
}

SSL_CTX *Socket_TLS::createClientSSLContext()
{
#if TLS_MAX_VERSION == TLS1_VERSION
    return SSL_CTX_new (TLSv1_client_method());
#elif TLS_MAX_VERSION == TLS1_1_VERSION
    return SSL_CTX_new (TLSv1_1_client_method());
#elif TLS_MAX_VERSION == TLS1_2_VERSION
    return SSL_CTX_new (TLSv1_2_client_method());
#elif TLS_MAX_VERSION >= TLS1_3_VERSION
    return SSL_CTX_new (TLS_client_method());
#endif
    return nullptr;
}

bool Socket_TLS::isServer() const
{
    return m_isServer;
}

bool Socket_TLS::isUsingPSK() const
{
    if (m_isServer)
        return m_tlsParentConnection->m_keys.getPSKServerWallet()->isUsingPSK;
    else
        return m_keys.getPSKClientValue()->isUsingPSK;
}


bool Socket_TLS::createTLSContext()
{
    // create new SSL Context.

    if (m_sslContext)
    {
        throw std::runtime_error("Can't reuse the TLS socket. Create a new one.");
        return false;
    }

    if (m_isServer)
    {
        m_sslContext = createServerSSLContext();
        if (!m_sslContext)
        {
            m_sslErrorList.push_back("TLS_server_method() Failed.");
            return false;
        }
    }
    else
    {
        m_sslContext = createClientSSLContext();
        if (!m_sslContext)
        {
            m_sslErrorList.push_back("TLS_client_method() Failed.");
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
        m_sslErrorList.push_back(buf);
    }
}

bool Socket_TLS::validateTLSConnection(const bool & usingPSK)
{
    if (!m_sslHandler)
        return false;

    bool bValid  = false;

    if (!usingPSK)
    {
        X509 *cert;
        cert = SSL_get_peer_certificate(m_sslHandler);
        if ( cert != nullptr )
        {
            long res = SSL_get_verify_result(m_sslHandler);
            if (res == X509_V_OK)
            {
                bValid = true;
            }
            else
            {
                m_sslErrorList.push_back("Peer TLS/SSL Certificate Verification Error (" + std::to_string(res) + "): " + std::string(X509_verify_cert_error_string(res)));
            }
            X509_free(cert);
        }
        else
            m_sslErrorList.push_back("Peer TLS/SSL Certificate does not exist.");
    }
    else
    {
        bValid = true;
    }

    return bValid;
}

Socket_TLS::eCertValidationOptions Socket_TLS::getCertValidation() const
{
    return m_certValidationOptions;
}

void Socket_TLS::setCertValidation(eCertValidationOptions newCertValidation)
{
    m_certValidationOptions = newCertValidation;
}


string Socket_TLS::getTLSConnectionCipherName()
{
    if (!m_sslHandler) return "";
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
        return "";

    if (!isUsingPSK())
    {
        char certCNText[512];
        memset(certCNText,0,sizeof(certCNText));

        X509 * cert = SSL_get_peer_certificate(m_sslHandler);
        if(cert)
        {
            X509_NAME * certName = X509_get_subject_name(cert);
            if (certName)
                X509_NAME_get_text_by_NID(certName,NID_commonName,certCNText,511);
            X509_free(cert);
        }
        return std::string(certCNText);
    }
    else
    {
        // the remote host is the server... no configured id.
        if (!m_isServer)
            return "server";
        else
            return m_keys.getPSKServerWallet()->connectedClientID;
    }
}

int Socket_TLS::iShutdown(int mode)
{
    if (!m_sslHandler && isServer())
    {
        // Then is a listening socket... (shutdown like a normal tcp/ip connection)
        return Socket::iShutdown(mode);
    }
    else if (!m_sslHandler)
    {
        return -4;
    }
    else if (m_shutdownProtocolOnWrite || (SSL_get_shutdown(m_sslHandler) & SSL_SENT_SHUTDOWN))
    {
        // Already shutted down.
        return -1;
    }
    else
    {

        // Messages from https://www.openssl.org/docs/manmaster/man3/SSL_shutdown.html
        switch (SSL_shutdown (m_sslHandler))
        {
        case 0:
            // The shutdown is not yet finished: the close_notify was sent but the peer did not send it back yet. Call SSL_read() to do a bidirectional shutdown.
            return -2;
        case 1:
            // The write shutdown was successfully completed. The close_notify alert was sent and the peer's close_notify alert was received.
            //shutdown_proto_rd = true;
            m_shutdownProtocolOnWrite = true;
            return 0;
            // Call the private TCP iShutdown to shutdown RD on the socket ?...
            //return Socket_TCP::iShutdown(mode);
        default:
            //The shutdown was not successful.
            return -3;
        }
    }
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
    if (!m_sslHandler) return cb;
    cb.asymmetricBits = SSL_get_cipher_bits(m_sslHandler, &cb.symmetricBits);
    return cb;
}

string Socket_TLS::getTLSConnectionProtocolVersion()
{
    if (!m_sslHandler) return "";
    return SSL_get_version(m_sslHandler);
}

std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream_Base> Socket_TLS::acceptConnection()
{
    char remotePair[INET6_ADDRSTRLEN];

    m_isServer = true;

    std::shared_ptr<Socket_Stream_Base>  acceptedTCPSock = Socket_TCP::acceptConnection();

    if (!acceptedTCPSock)
        return nullptr;

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
    acceptedTLSSock->setSocketFD(acceptedTCPSock->adquireSocketFD());
    //delete acceptedTCPSock;

    // After this, the postInitialization will be called by the acceptor thread.
    return acceptedTLSSock;
}

int Socket_TLS::partialRead(void * data, const uint32_t & datalen)
{
    if (!m_sslHandler) return -1;
    char cError[1024]="Unknown Error";

    int readBytes = SSL_read(m_sslHandler, data, datalen);
    if (readBytes > 0)
    {
        return readBytes;
    }
    else
    {
        // Connection may be lost... shutdown here using the TCP/IP layer to prevent further protocol negotiations that can lead to sigpipe...
        Socket_TCP::iShutdown();

        int err = SSL_get_error(m_sslHandler, readBytes);
        switch(err)
        {
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_READ:
            parseErrors();
            return -1;
        case SSL_ERROR_ZERO_RETURN:
            // Socket closed.
            parseErrors();
            return -1;
        case SSL_ERROR_SYSCALL:
            // Common error (maybe tcp error).
            parseErrors();
            m_lastError = "Error " + std::to_string(errno) + " during read: " + strerror_r(errno,cError,sizeof(cError));
            return -1;
        default:
            parseErrors();
            return -1;
        }
    }
}

int Socket_TLS::partialWrite(const void * data, const uint32_t & datalen)
{
    if (!m_sslHandler) return -1;

    // TODO: sigpipe here?
    int sentBytes = SSL_write(m_sslHandler, data, datalen);
    if (sentBytes > 0)
    {
        return sentBytes;
    }
    else
    {
        // Connection may be lost... shutdown here using the TCP/IP layer to prevent further protocol negotiations that can lead to sigpipe...
        Socket_TCP::iShutdown();

        switch(SSL_get_error(m_sslHandler, sentBytes))
        {
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_READ:
            // Must wait a little bit until the socket buffer is free
            // TODO: ?
            usleep(100000);
            return 0;
        case SSL_ERROR_ZERO_RETURN:
            // Socket closed...
            parseErrors();
            return -1;
        default:
            // Another SSL Error.
            parseErrors();
            return -1;
        }
    }
}

