#include "socket_tls.h"

#include "streamsocket.h"

#include <openssl/rand.h>
#include <openssl/err.h>

#include <iostream>

#include <unistd.h>
#include <signal.h>

#include <limits>

#ifdef _WIN32
#include <openssl/safestack.h>
#endif

using namespace std;
using namespace Mantids::Network::TLS;

Socket_TLS::Socket_TLS()
{
    acceptInvalidServerCerts = false;
    isServer = false;
    sslHandle = nullptr;
    sslContext = nullptr;
}

Socket_TLS::~Socket_TLS()
{
    if (sslHandle)
    {
        SSL_free (sslHandle);
    }
    if (sslContext)
    {
        SSL_CTX_free(sslContext);
    }
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

bool Socket_TLS::postConnectSubInitialization()
{
    if (sslHandle!=nullptr) return false; // already connected (don't connect again)

    isServer = false;

    if (!tlsInitContext())
    {
        return false;
    }

    if (!(sslHandle = SSL_new(sslContext)))
    {
        sslErrors.push_back("SSL_new failed.");
        return false;
    }

    if (SSL_set_fd (sslHandle, sockfd) != 1)
    {
        sslErrors.push_back("SSL_set_fd failed.");
        return false;
    }

    if ( SSL_get_error(sslHandle, SSL_connect (sslHandle)) != SSL_ERROR_NONE )
    {
        parseErrors();
        return false;
    }

    if (!acceptInvalidServerCerts && !validateConnection())
    {
        return false;
    }

    // connected!
    return true;
}

bool Socket_TLS::postAcceptSubInitialization()
{
    if (sslHandle!=nullptr) return false; // already connected (don't connect again)

    isServer = true;

    if (!tlsInitContext())
    {
        return false;
    }

    // ssl empty, create a new one.
    if (!(sslHandle = SSL_new(sslContext)))
    {
        sslErrors.push_back("SSL_new failed.");
        return false;
    }

    if ( ca_file.size()>0 )
        SSL_set_verify(sslHandle, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);

    if (SSL_set_fd (sslHandle, sockfd) != 1)
    {
        sslErrors.push_back("SSL_set_fd failed.");
        return false;
    }

    if (SSL_accept(sslHandle) != 1)
    {
        parseErrors();
        return false;
    }

    // connected!
    return true;
}

bool Socket_TLS::tlsInitContext()
{
    // create new SSL Context.

    if (isServer)
    {
#if TLS_MAX_VERSION == TLS1_VERSION
        sslContext = SSL_CTX_new (TLSv1_server_method());
#elif TLS_MAX_VERSION == TLS1_1_VERSION
        sslContext = SSL_CTX_new (TLSv1_1_server_method());
#elif TLS_MAX_VERSION == TLS1_2_VERSION
        sslContext = SSL_CTX_new (TLSv1_2_server_method());
#elif TLS_MAX_VERSION == TLS1_3_VERSION
        sslContext = SSL_CTX_new (TLS_server_method());
#endif
        if (!sslContext)
        {
            sslErrors.push_back("SSL_CTX_new Failed in server mode.");
            return false;
        }
    }
    else
    {
#if TLS_MAX_VERSION == TLS1_VERSION
        sslContext = SSL_CTX_new (TLSv1_client_method());
#elif TLS_MAX_VERSION == TLS1_1_VERSION
        sslContext = SSL_CTX_new (TLSv1_1_client_method());
#elif TLS_MAX_VERSION == TLS1_2_VERSION
        sslContext = SSL_CTX_new (TLSv1_2_client_method());
#elif TLS_MAX_VERSION == TLS1_3_VERSION
        sslContext = SSL_CTX_new (TLS_client_method());
#endif
        if (!sslContext)
        {
            sslErrors.push_back("SSL_CTX_new Failed in client mode.");
            return false;
        }
    }

    if (!ca_file.empty())
    {
        if (SSL_CTX_load_verify_locations(sslContext, ca_file.c_str(),nullptr) != 1)
        {
            sslErrors.push_back("SSL_CTX_load_verify_locations Failed for CA.");
            return false;
        }
        STACK_OF(X509_NAME) *list;
        list = SSL_load_client_CA_file( ca_file.c_str() );
        if( list != nullptr )
        {
            SSL_CTX_set_client_CA_list( sslContext, list );
            // It takes ownership. (list now belongs to sslContext)
        }
    }

    if (!crt_file.empty() && (SSL_CTX_use_certificate_file(sslContext, crt_file.c_str(), SSL_FILETYPE_PEM) != 1))
    {
        sslErrors.push_back("SSL_CTX_use_certificate_file Failed for local Certificate.");
        return false;
    }
    if (!key_file.empty() && (SSL_CTX_use_PrivateKey_file(sslContext, key_file.c_str(), SSL_FILETYPE_PEM) != 1))
    {
        sslErrors.push_back("SSL_CTX_use_PrivateKey_file Failed for private key.");
        return false;
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
        sslErrors.push_back(buf);
    }
}

bool Socket_TLS::validateConnection()
{
    if (!sslHandle) return false;
    X509 *cert;
    bool bValid  = false;
    cert = SSL_get_peer_certificate(sslHandle);
    if ( cert != nullptr )
    {
        long res = SSL_get_verify_result(sslHandle);
        if (res == X509_V_OK)
        {
            bValid = true;
        }
        else
        {
            sslErrors.push_back("TLS/SSL Certificate Error: " + std::to_string(res));
        }
        X509_free(cert);
    }

    return bValid;
}

bool Socket_TLS::getAcceptInvalidServerCerts() const
{
    return acceptInvalidServerCerts;
}

void Socket_TLS::setAcceptInvalidServerCerts(bool newAcceptInvalidServerCerts)
{
    acceptInvalidServerCerts = newAcceptInvalidServerCerts;
}

string Socket_TLS::getCipherName()
{
    if (!sslHandle) return "";
    return SSL_get_cipher_name(sslHandle);
}

std::list<std::string> Socket_TLS::getTLSErrorsAndClear()
{
    std::list<std::string> sslErrors2 = sslErrors;
    sslErrors.clear();
    return sslErrors2;
}

string Socket_TLS::getTLSPeerCN()
{
    if (!sslHandle) return "";
    char certCNText[512]="";
    X509 * cert = SSL_get_peer_certificate(sslHandle);
    if(cert)
    {
        X509_NAME * certName = X509_get_subject_name(cert);
        if (certName)
        {
            X509_NAME_get_text_by_NID(certName,NID_commonName,certCNText,511);
        }
        X509_free(cert);
    }
    return std::string(certCNText);
}

int Socket_TLS::iShutdown(int mode)
{
    if (!sslHandle) return -2;

    if (shutdown_proto_wr)
    {
        // Already shutted down.
        return -1;
    }

    // Messages from https://www.openssl.org/docs/manmaster/man3/SSL_shutdown.html
    switch (SSL_shutdown (sslHandle))
    {
    case 0:
        // The shutdown is not yet finished: the close_notify was sent but the peer did not send it back yet. Call SSL_read() to do a bidirectional shutdown.
        return -2;
    case 1:
        // The shutdown was successfully completed. The close_notify alert was sent and the peer's close_notify alert was received.
        //shutdown_proto_rd = true;
        shutdown_proto_wr = true;
        // Call the private iShutdown to shutdown RD on the socket...
        return Socket::iShutdown(mode);
    default:
        //The shutdown was not successful.
        return -3;
    }
}

bool Socket_TLS::isSecure() { return true; }

std::string Socket_TLS::getCertificateAuthorityPath() const
{
    return ca_file;
}

std::string Socket_TLS::getPrivateKeyPath() const
{
    return key_file;
}

std::string Socket_TLS::getPublicKeyPath() const
{
    return crt_file;
}

void Socket_TLS::setServerMode(bool value)
{
    isServer = value;
}

void Socket_TLS::setTLSContextMode(const SSL_MODE &value)
{
    sslMode = value;
}

bool Socket_TLS::setTLSCertificateAuthorityPath(const char *_ca_file)
{
    if (access(_ca_file,R_OK)) return false;
    ca_file = _ca_file;
    return true;
}

bool Socket_TLS::setTLSPublicKeyPath(const char *_crt_file)
{
    if (access(_crt_file,R_OK)) return false;
    crt_file = _crt_file;
    return true;
}

bool Socket_TLS::setTLSPrivateKeyPath(const char *_key_file)
{
    if (access(_key_file,R_OK)) return false;
    key_file = _key_file;
    return true;
}


cipherBits Socket_TLS::getCipherBits()
{
    cipherBits cb;
    if (!sslHandle) return cb;
    cb.aSymBits = SSL_get_cipher_bits(sslHandle, &cb.symBits);
    return cb;
}

string Socket_TLS::getProtocolVersionName()
{
    if (!sslHandle) return "";
    return SSL_get_version(sslHandle);
}

Mantids::Network::Streams::StreamSocket * Socket_TLS::acceptConnection()
{
    char remotePair[INET6_ADDRSTRLEN];
    StreamSocket * mainSock = Socket_TCP::acceptConnection();
    if (!mainSock) return nullptr;


    Socket_TLS * tlsSock = new Socket_TLS; // Convert to this thing...
    isServer = true;

    // Set current retrieved socket info.
    mainSock->getRemotePair(remotePair);
    tlsSock->setRemotePair(remotePair);
    tlsSock->setRemotePort(mainSock->getRemotePort());

    // Set certificates paths...
    if (!ca_file.empty()) tlsSock->setTLSCertificateAuthorityPath( ca_file.c_str() );
    if (!crt_file.empty()) tlsSock->setTLSPublicKeyPath( crt_file.c_str() );
    if (!key_file.empty()) tlsSock->setTLSPrivateKeyPath( key_file.c_str() );

    // Set contexts and modes...
    tlsSock->setTLSContextMode(sslMode);
    tlsSock->setServerMode(isServer);

    // now we should copy the file descriptor:
    tlsSock->setSocketFD(mainSock->adquireSocketFD());
    delete mainSock;

    // TODO: where is called postAcceptsubinitialization?

    return tlsSock;
}

int Socket_TLS::partialRead(void * data, const uint32_t & datalen)
{
    if (!sslHandle) return -1;
    char cError[1024]="Unknown Error";

    int readBytes = SSL_read(sslHandle, data, datalen);
    if (readBytes > 0)
    {
        return readBytes;
    }
    else
    {
        int err = SSL_get_error(sslHandle, readBytes);
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
            lastError = "Error " + std::to_string(errno) + " during read: " + strerror_r(errno,cError,sizeof(cError));
            return -1;
        default:
            parseErrors();
            return -1;
        }
    }
}

int Socket_TLS::partialWrite(const void * data, const uint32_t & datalen)
{
    if (!sslHandle) return -1;

    // TODO: sigpipe here?
    int sentBytes = SSL_write(sslHandle, data, datalen);
    if (sentBytes > 0)
    {
        return sentBytes;
    }
    else
    {
        switch(SSL_get_error(sslHandle, sentBytes))
        {
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_READ:
            // Must wait a little bit until the socket buffer is free
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


