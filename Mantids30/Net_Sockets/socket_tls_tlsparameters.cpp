#include "socket_tls.h"
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <string.h>
#include <Mantids30/Memory/streamablefile.h>
#include <Mantids30/Helpers/random.h>
#include <stdexcept>

using namespace std;
using namespace Mantids30::Network::Sockets;

#ifdef _WIN32
#define FS_DIRSLASH "\\"
#else
#define FS_DIRSLASH "/"
#endif


Socket_TLS::TLSKeyParameters::TLSKeyParameters(bool * isServer) : m_pskStaticHandler(&m_pskClientValues, &m_pskServerValues)
{
#if TLS_MAX_VERSION == TLS1_VERSION
    maxProtocolVersion = TLS1_VERSION;
#elif TLS_MAX_VERSION == TLS1_1_VERSION
    maxProtocolVersion = TLS1_1_VERSION;
#elif TLS_MAX_VERSION == TLS1_2_VERSION
    maxProtocolVersion = TLS1_2_VERSION;
#elif TLS_MAX_VERSION == TLS1_3_VERSION
    // I want to stick to 1.2 (unless will be requested in the api)
    m_maxProtocolVersion = TLS1_2_VERSION;
#elif TLS_MAX_VERSION > TLS1_3_VERSION
    // Not established, unknown by now, use defaults...
    maxProtocolVersion = -1;
#endif

    // Set the server parameter from the parent.
    this->m_isServer = isServer;

    // Set the default DH parameter as 4096bit
    m_dhParameter =get_dh4096();
}

Socket_TLS::TLSKeyParameters::~TLSKeyParameters()
{
    if (m_dhParameter)
        DH_free(m_dhParameter);
    if (m_privateKey)
        EVP_PKEY_free(m_privateKey);
    if (m_publicKey)
        X509_free(m_publicKey);

    // Remove the PSK from the static list...
    if (!m_TLSCertificateAuthorityMemory.empty())
    {
        remove(m_TLSCertificateAuthorityPath.c_str());
    }
}

void Socket_TLS::TLSKeyParameters::setUsingPSK()
{
    // For TLSv1.2 use only secure PSK algorithms:
    m_TLSCipherList = "DHE-PSK-AES256-GCM-SHA384:DHE-PSK-AES128-GCM-SHA256";
}

void Socket_TLS::TLSKeyParameters::addPSKToServer(const std::string &clientIdentity,const std::string &_psk)
{
    setUsingPSK();
    m_pskServerValues.setPSKByClientID(clientIdentity,_psk);
}

void Socket_TLS::TLSKeyParameters::loadPSKAsClient(const std::string &clientIdentity, const std::string &_psk)
{
    setUsingPSK();
    m_pskClientValues.setValues(clientIdentity,_psk);
}

bool Socket_TLS::TLSKeyParameters::linkPSKWithTLSHandle(SSL *_sslh)
{
    return this->m_pskStaticHandler.setSSLHandler(_sslh);
}

#define ERR_ON_ZERO( func, err ) { if ( func == 0 ) \
                                      { \
                                      keyErrors->push_back(err); \
                                      return false; \
                                      }\
                                 }

bool Socket_TLS::TLSKeyParameters::initTLSKeys( SSL_CTX *ctx, SSL *sslh, std::list<std::string> * keyErrors )
{
#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    if (m_securityLevel!=-1)
        SSL_set_security_level(sslh,m_securityLevel);
    if (m_maxProtocolVersion!=-1)
        SSL_set_max_proto_version(sslh,m_maxProtocolVersion);
    if (m_minProtocolVersion!=-1)
        SSL_set_min_proto_version(sslh,m_minProtocolVersion);

    SSL_clear_options(sslh,SSL_OP_PRIORITIZE_CHACHA);
    SSL_clear_options(sslh,SSL_OP_ALLOW_NO_DHE_KEX);

#else
    if ( minProtocolVersion >= TLS1_VERSION )
        SSL_set_options(sslh, SSL_OP_NO_SSLv3);

    if ( minProtocolVersion >= TLS1_1_VERSION )
        SSL_set_options(sslh, SSL_OP_NO_TLSv1);

    if ( minProtocolVersion >= TLS1_2_VERSION )
        SSL_set_options(sslh, SSL_OP_NO_TLSv1_1);

    if ( maxProtocolVersion < TLS1_2_VERSION )
        SSL_set_options(sslh, SSL_OP_NO_TLSv1_2);

    if ( maxProtocolVersion < TLS1_1_VERSION )
        SSL_set_options(sslh, SSL_OP_NO_TLSv1_1);

    if ( maxProtocolVersion < TLS1_VERSION )
        SSL_set_options(sslh, SSL_OP_NO_TLSv1);
#endif


    if (!*m_isServer)
    {
        SSL_set_options(sslh, SSL_OP_CIPHER_SERVER_PREFERENCE);
    }


    // Validate:
    if ( m_publicKey && !m_privateKey )
    {
        keyErrors->push_back("If there is a X.509 certificate, a private key must be provided.");
        return false;
    }
    else if ( !m_publicKey && m_privateKey )
    {
        keyErrors->push_back("If there is a X.509 private key, a certificate key must be provided.");
        return false;
    }

    if (!m_TLSCertificateAuthorityPath.empty())
    {
        ERR_ON_ZERO( SSL_CTX_load_verify_locations(ctx, m_TLSCertificateAuthorityPath.c_str(),nullptr), "SSL_load_verify_locations Failed for CA.");
        STACK_OF(X509_NAME) *list;
        list = SSL_load_client_CA_file( m_TLSCertificateAuthorityPath.c_str() );
        if( list != nullptr )
        {
            // It takes ownership. (list now belongs to sslContext, no need to free)
            SSL_set_client_CA_list( sslh, list );
            if (m_maxVerifyDepth >=0 -1)
                SSL_set_verify_depth( sslh, m_maxVerifyDepth);
        }
        // TODO: warn if the list is zero.
    }

    if (m_useSystemCertificates)
    {
        ERR_ON_ZERO( SSL_CTX_set_default_verify_paths(ctx), "SSL_CTX_set_default_verify_paths Failed.");
    }

    // Setup Diffie-Hellman
    ERR_ON_ZERO(m_dhParameter && SSL_set_tmp_dh(sslh,m_dhParameter), "SSL_set_tmp_dh Failed for you temporary DH key.");

    //SSL_set_dh_auto(sslh,1);
    SSL_set_ecdh_auto(sslh,1);

    // TLSv1.3 parameters:
#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    ERR_ON_ZERO(!m_TLSSharedGroups.empty() && SSL_set1_groups_list(sslh,m_TLSSharedGroups.c_str()), "SSL_set1_groups_list Failed for your shared groups.");
    ERR_ON_ZERO(!m_TLSCipherSuites.empty() && SSL_set_ciphersuites(sslh,m_TLSCipherSuites.c_str()), "SSL_set_ciphersuites Failed for your cipher suites.");
#endif
    // TLSv1.2 Cipher List
    ERR_ON_ZERO(!m_TLSCipherList.empty() && SSL_set_cipher_list( sslh, m_TLSCipherList.c_str()), "SSL_set_cipher_list Failed for your cipher list.");

    bool usingPSK = (m_pskClientValues.m_isUsingPSK || m_pskServerValues.m_isUsingPSK);

    if (!usingPSK)
    {
        // X509 mode.
        // Setup CRT/KEY for file:
        ERR_ON_ZERO(m_publicKey && SSL_use_certificate( sslh, m_publicKey), "SSL_use_certificate Failed for local Certificate.");
        ERR_ON_ZERO(m_privateKey && SSL_use_PrivateKey( sslh, m_privateKey), "SSL_use_PrivateKey Failed for private key.");
    }
    else
    {
        // PSK Mode.
        if (!*m_isServer) // Client identity is set up in the callback:
            SSL_set_psk_client_callback( sslh, cbPSKClient );
        else // Server receives the identity from client:
            SSL_set_psk_server_callback( sslh, cbPSKServer );
    }

    return true;
}

bool Socket_TLS::TLSKeyParameters::getValidateServerHostname() const
{
    return m_validateServerHostname;
}

void Socket_TLS::TLSKeyParameters::setValidateServerHostname(bool newValidateServerHostname)
{
    m_validateServerHostname = newValidateServerHostname;
}

unsigned int Socket_TLS::TLSKeyParameters::cbPSKServer(SSL *ssl, const char *identity, unsigned char *psk, unsigned int max_psk_len)
{
    auto * pskValues = PSKStaticHdlr::getServerWallet(ssl);

    // No registered clients... (???)
    if (!pskValues)
        return 0;

    strncpy((char *)psk,"",max_psk_len);

    std::string _psk;

    // Using callback strategy if callback is defined, otherwise use the local map...
    if ( (pskValues->m_pskCallback? pskValues->m_pskCallback(pskValues->m_data,identity,&_psk) : pskValues->getPSKByClientID(identity,&_psk)) )
    {
        // Set the provided ID.
        pskValues->m_connectedClientID = identity;

        // return the proper key for the iPSK negotiation.
        snprintf((char *)psk,max_psk_len,"%s",_psk.c_str());
        return strlen((char *)psk);
    }
    // No ID found.
    return 0;
}

unsigned int Socket_TLS::TLSKeyParameters::cbPSKClient(SSL *ssl, const char *hint, char *identity, unsigned int max_identity_len, unsigned char *psk, unsigned int max_psk_len)
{  
    auto * pskValue = PSKStaticHdlr::getClientValue(ssl);
    snprintf((char *)psk,max_psk_len,"%s",  pskValue->m_psk.c_str() );
    snprintf(identity,max_identity_len,"%s", pskValue->m_identity.c_str() );
    return strlen((char *)psk);
}

Socket_TLS::TLSKeyParameters::PSKClientValue *Socket_TLS::TLSKeyParameters::getPSKClientValue()
{
    return &m_pskClientValues;
}

Socket_TLS::TLSKeyParameters::PSKServerWallet *Socket_TLS::TLSKeyParameters::getPSKServerWallet()
{
    return &m_pskServerValues;
}

bool Socket_TLS::TLSKeyParameters::loadPrivateKeyFromPEMFile(const char *filePath, pem_password_cb * cbPass, void *u)
{
    bool r=false;
    FILE *fp = fopen(filePath,"r");
    if (fp)
    {
        if (m_privateKey)
            EVP_PKEY_free(m_privateKey);
        m_privateKey = nullptr;
        PEM_read_PrivateKey(fp,&m_privateKey, cbPass, u );
        if (m_privateKey)
            r=true;
        fclose(fp);
    }
    return r;
}

bool Socket_TLS::TLSKeyParameters::loadPublicKeyFromPEMFile(const char *filePath, pem_password_cb * cbPass, void *u)
{
    bool r=false;
    FILE *fp = fopen(filePath,"r");
    if (fp)
    {
        if (m_publicKey)
            X509_free(m_publicKey);
        m_publicKey = nullptr;
        PEM_read_X509(fp,&m_publicKey, cbPass, u );
        if (m_publicKey)
            r=true;
        fclose(fp);
    }
    return r;
}

bool Socket_TLS::TLSKeyParameters::loadPrivateKeyFromPEMMemory(const char *privKeyPEMData, pem_password_cb * cbPass, void *u)
{
    bool r=false;
    BIO * buf = BIO_new_mem_buf( privKeyPEMData, strlen(privKeyPEMData));
    if (buf)
    {
        if (m_privateKey)
            EVP_PKEY_free(m_privateKey);
        m_privateKey = nullptr;
        PEM_read_bio_PrivateKey(buf,&m_privateKey, cbPass, u );
        if (m_privateKey)
            r=true;
        BIO_free(buf);
    }
    return r;
}

bool Socket_TLS::TLSKeyParameters::loadPublicKeyFromPEMMemory(const char *pubKeyPEMData, pem_password_cb * cbPass, void *u)
{
    bool r=false;
    BIO * buf = BIO_new_mem_buf( pubKeyPEMData, strlen(pubKeyPEMData));
    if (buf)
    {
        if (m_publicKey)
            X509_free(m_publicKey);
        m_publicKey = nullptr;
        PEM_read_bio_X509(buf,&m_publicKey, cbPass, u );
        if (m_publicKey)
            r=true;
        BIO_free(buf);
    }
    return r;
}

const std::string &Socket_TLS::TLSKeyParameters::getTLSCipherList() const
{
    return m_TLSCipherList;
}

void Socket_TLS::TLSKeyParameters::setTLSCipherList(const std::string &newSTLSCipherList)
{
    m_TLSCipherList = newSTLSCipherList;
}

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
DH *Socket_TLS::TLSKeyParameters::get_dh4096()
{
    static unsigned char dhp_4096[] = {
        0xEC, 0x30, 0xD9, 0xC7, 0x4F, 0x53, 0xB4, 0x3D, 0x91, 0x24,
        0xFF, 0xA2, 0x8E, 0x4D, 0x1D, 0x89, 0x6E, 0x9B, 0xCC, 0xD4,
        0x2B, 0x09, 0x49, 0xCE, 0xB4, 0x23, 0x1D, 0x05, 0x2A, 0x0D,
        0x14, 0x00, 0x1A, 0x06, 0x62, 0x00, 0x18, 0x7B, 0xFA, 0x50,
        0x4A, 0xD9, 0xE3, 0xBD, 0x17, 0x92, 0x30, 0x79, 0x60, 0x54,
        0x31, 0xDF, 0xB4, 0x7C, 0x6C, 0xB2, 0x2D, 0xEC, 0x87, 0x41,
        0x5E, 0xE8, 0xBA, 0x03, 0xD4, 0x08, 0xF9, 0xD3, 0x81, 0x1F,
        0xFC, 0xB0, 0xF1, 0xA5, 0x12, 0xD1, 0x8D, 0x20, 0xC6, 0xED,
        0xB3, 0xDC, 0x8A, 0x27, 0x03, 0xF7, 0x64, 0x77, 0x88, 0x13,
        0x78, 0x20, 0xEA, 0x6D, 0x87, 0x26, 0xA9, 0x68, 0x61, 0x3E,
        0x1A, 0x19, 0xCF, 0xEC, 0xA2, 0x2D, 0x70, 0xCF, 0x19, 0xB8,
        0xF0, 0xF9, 0xBD, 0xCE, 0xA3, 0x8A, 0xCE, 0x51, 0x4C, 0x24,
        0xF4, 0x7F, 0xE8, 0xA2, 0x1E, 0x8F, 0xC9, 0xEE, 0x56, 0xB2,
        0x5C, 0xF8, 0x72, 0xC3, 0x0D, 0x04, 0x33, 0xD7, 0xE9, 0xB3,
        0x6A, 0xF9, 0x07, 0x36, 0x8E, 0x67, 0x62, 0x40, 0xB1, 0x39,
        0xA4, 0x27, 0xA8, 0x86, 0x54, 0x7F, 0x4C, 0xFD, 0x86, 0x0C,
        0x4E, 0x4C, 0x8E, 0x2E, 0xBC, 0x27, 0x9C, 0x30, 0x33, 0x59,
        0x82, 0x33, 0xBE, 0xA9, 0xC2, 0x81, 0x9F, 0x8A, 0x9B, 0x58,
        0xD6, 0x90, 0x49, 0x50, 0x56, 0x27, 0xD0, 0x38, 0x04, 0xC8,
        0xD9, 0x55, 0x29, 0xBC, 0x31, 0x2C, 0xFB, 0xB8, 0xB4, 0x49,
        0x76, 0x2F, 0xFB, 0xBF, 0xBC, 0xC0, 0x50, 0x42, 0x84, 0xEB,
        0xA0, 0xF8, 0x8A, 0x2E, 0xF3, 0xC0, 0x52, 0xFB, 0x29, 0x37,
        0x4B, 0xEE, 0x2E, 0xB5, 0xB0, 0xE0, 0xA8, 0x71, 0x0F, 0x9B,
        0xB5, 0x76, 0x3A, 0xD1, 0x7A, 0x5B, 0x6A, 0xD2, 0x70, 0x55,
        0x03, 0xC7, 0x09, 0xA6, 0xA1, 0xD1, 0xCE, 0x97, 0x08, 0x82,
        0xCC, 0xFE, 0x94, 0x54, 0x8B, 0x19, 0x22, 0xDE, 0x36, 0xF2,
        0x43, 0xB8, 0xA1, 0x68, 0x76, 0x67, 0x63, 0x39, 0x4B, 0xEB,
        0x4B, 0xBE, 0x3A, 0x00, 0x87, 0x44, 0xFD, 0x1F, 0xBE, 0x27,
        0xEE, 0x46, 0x6C, 0x4A, 0x99, 0x36, 0xE2, 0xD3, 0x97, 0x80,
        0xE9, 0xE8, 0x5E, 0x5B, 0xE2, 0x55, 0x13, 0xD1, 0x02, 0xBD,
        0x54, 0xBB, 0x00, 0x90, 0xE3, 0x03, 0x4C, 0x99, 0x0F, 0xE1,
        0x67, 0x2E, 0xC8, 0xD2, 0xA3, 0xA4, 0x90, 0x89, 0xA0, 0xDE,
        0x95, 0x40, 0xC9, 0xCE, 0x62, 0x97, 0x00, 0x0A, 0xAC, 0x83,
        0xFF, 0xAC, 0xB5, 0x58, 0x34, 0x6D, 0xCD, 0x56, 0x7C, 0xA0,
        0xDE, 0xC5, 0x0A, 0x7B, 0x98, 0x3D, 0xD2, 0x06, 0x4E, 0xF9,
        0xBF, 0x64, 0xD5, 0x3D, 0x5D, 0x79, 0x08, 0x8D, 0xD4, 0xCD,
        0xAF, 0x66, 0x30, 0x01, 0xED, 0xB5, 0x08, 0x02, 0x4A, 0xDE,
        0x93, 0xDF, 0x30, 0xF4, 0x9E, 0xD3, 0x75, 0x87, 0xE0, 0x84,
        0x52, 0x7D, 0x7F, 0xBD, 0x62, 0xBC, 0x02, 0x9B, 0xDB, 0xD6,
        0x64, 0xF1, 0x29, 0xBC, 0x7E, 0xF2, 0x96, 0x36, 0xA6, 0xBB,
        0xA9, 0x47, 0xA9, 0xEF, 0x63, 0x54, 0xF9, 0xC0, 0xE4, 0x69,
        0x34, 0xB7, 0x28, 0x6F, 0x68, 0xCA, 0x89, 0xF1, 0xA4, 0x41,
        0x61, 0xD6, 0x9A, 0xF7, 0xBC, 0x71, 0x57, 0x7C, 0x77, 0xB8,
        0x27, 0x7F, 0xB5, 0x35, 0xD7, 0x2A, 0x28, 0x09, 0xD6, 0x80,
        0x36, 0xD1, 0x2C, 0xB6, 0x27, 0x50, 0xA4, 0x2C, 0x12, 0x50,
        0x8F, 0x91, 0x3F, 0xDF, 0xD0, 0x76, 0xB0, 0x06, 0x5D, 0xC9,
        0x88, 0x01, 0x2C, 0x0D, 0x0C, 0x44, 0x10, 0x67, 0xBD, 0xE6,
        0xF3, 0xC7, 0xBB, 0x26, 0x65, 0x76, 0x6F, 0xF9, 0x48, 0x5E,
        0x87, 0xD1, 0xF0, 0xE9, 0x02, 0xF7, 0x78, 0x00, 0x93, 0xF6,
        0xC8, 0x97, 0x0A, 0x50, 0x5C, 0x44, 0x33, 0x6F, 0x7B, 0x89,
        0x88, 0x1A, 0x5C, 0xC7, 0x16, 0x93, 0x7B, 0x68, 0xBB, 0x30,
        0xDE, 0x93
    };
    static unsigned char dhg_4096[] = {
        0x02
    };
    DH *dh = DH_new();
    BIGNUM *p, *g;

    if (dh == NULL)
        return NULL;
    p = BN_bin2bn(dhp_4096, sizeof(dhp_4096), NULL);
    g = BN_bin2bn(dhg_4096, sizeof(dhg_4096), NULL);
    if (p == NULL || g == NULL
            || !DH_set0_pqg(dh, p, NULL, g)) {
        DH_free(dh);
        BN_free(p);
        BN_free(g);
        return NULL;
    }
    return dh;
}
#else
DH *Socket_TLS::TLSKeyParameters::get_dh4096() { return nullptr; }
#endif

int Socket_TLS::TLSKeyParameters::getSecurityLevel() const
{
    return m_securityLevel;
}

void Socket_TLS::TLSKeyParameters::setSecurityLevel(int newSecurityLevel)
{
    m_securityLevel = newSecurityLevel;
}
/*
const std::string &Socket_TLS::TLSKeyParameters::getPSK() const
{
    std::unique_lock<std::mutex> lock(mPskBySSLHandlerPTR);
    return sPSK;
}
*/
int Socket_TLS::TLSKeyParameters::getVerifyMaxDepth() const
{
    return m_maxVerifyDepth;
}

void Socket_TLS::TLSKeyParameters::setVerifyMaxDepth(int newIVerifyMaxDepth)
{
    m_maxVerifyDepth = newIVerifyMaxDepth;
}

const std::string &Socket_TLS::TLSKeyParameters::getCAPath() const
{
    return m_TLSCertificateAuthorityPath;
}

bool Socket_TLS::TLSKeyParameters::loadCAFromPEMFile(const std::string &newSTLSCertificateAuthorityPath)
{
    if (!m_TLSCertificateAuthorityMemory.empty())
        throw std::runtime_error("Can't load a CA from path if there is a established CA memory.");

    if ( !access(newSTLSCertificateAuthorityPath.c_str(),R_OK) )
    {
        m_TLSCertificateAuthorityPath = newSTLSCertificateAuthorityPath;
        return true;
    }
    return false;
}

bool Socket_TLS::TLSKeyParameters::loadCAFromPEMMemory(const char *caCrtPEMData, const char * suffix)
{
    if (!m_TLSCertificateAuthorityPath.empty())
        throw std::runtime_error("Can't load a CA into memory if there is a established CA path.");

    m_TLSCertificateAuthorityMemory = caCrtPEMData;
    std::string fsDirectoryPath;

#ifdef _WIN32
    char tempPath[MAX_PATH+1];
    GetTempPathA(MAX_PATH,tempPath);
    fsDirectoryPath = tempPath;
#else
    fsDirectoryPath = "/tmp";
#endif

    m_TLSCertificateAuthorityPath =
            fsDirectoryPath + std::string(FS_DIRSLASH) + "ca_" + (suffix? std::string(suffix) : Mantids30::Helpers::Random::createRandomHexString(8)) + ".crt";

    Memory::Streams::StreamableFile sFile;
    if (sFile.open( m_TLSCertificateAuthorityPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600 )==-1)
    {
        m_TLSCertificateAuthorityPath = "";
        m_TLSCertificateAuthorityMemory = "";
        remove(m_TLSCertificateAuthorityPath.c_str());
        return false;
    }
    sFile.writeString(m_TLSCertificateAuthorityMemory);

    return true;
}


int Socket_TLS::TLSKeyParameters::getMinProtocolVersion() const
{
    return m_minProtocolVersion;
}

void Socket_TLS::TLSKeyParameters::setMinProtocolVersion(int newMinProtocolVersion)
{
    m_minProtocolVersion = newMinProtocolVersion;
}

int Socket_TLS::TLSKeyParameters::getMaxProtocolVersion() const
{
    return m_maxProtocolVersion;
}

void Socket_TLS::TLSKeyParameters::setMaxProtocolVersion(int newMaxProtocolVersion)
{
    m_maxProtocolVersion = newMaxProtocolVersion;
}

bool Socket_TLS::TLSKeyParameters::getUseSystemCertificates() const
{
    return m_useSystemCertificates;
}

void Socket_TLS::TLSKeyParameters::setUseSystemCertificates(bool newBVerifyDefaultLocations)
{
    m_useSystemCertificates = newBVerifyDefaultLocations;
}

const std::string &Socket_TLS::TLSKeyParameters::getTLSSharedGroups() const
{
    return m_TLSSharedGroups;
}

void Socket_TLS::TLSKeyParameters::setTLSSharedGroups(const std::string &newSTLSSharedGroups)
{
    m_TLSSharedGroups = newSTLSSharedGroups;
}

const std::string &Socket_TLS::TLSKeyParameters::getSTLSCipherSuites() const
{
    return m_TLSCipherSuites;
}

void Socket_TLS::TLSKeyParameters::setSTLSCipherSuites(const std::string &newSTLSCipherSuites)
{
    m_TLSCipherSuites = newSTLSCipherSuites;
}
