#include "socket_chain_aes.h"
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <random>

#ifdef _WIN32
//#pragma comment(lib, "crypt32.lib")
#include <windows.h>
#include <wincrypt.h>
#endif

#include <Mantids30/Helpers/mem.h>

using namespace Mantids30::Network::Sockets::ChainProtocols;

using namespace std;

const EVP_CIPHER * Socket_Chain_AES::m_cipher = openSSLInit();

Socket_Chain_AES::Socket_Chain_AES()
{
    setAESRegenBlockSize();
}


void Socket_Chain_AES::setAESRegenBlockSize(const size_t &value)
{
    m_aesRegenBlockSize = value;
}

void Socket_Chain_AES::setPhase1Key256(const char *pass)
{
    memcpy(m_phase1Key,pass,sizeof(m_phase1Key));
}

void Socket_Chain_AES::setPhase1Key(const char *pass)
{
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, pass, strlen(pass));
    SHA256_Final((unsigned char *)m_phase1Key, &sha256);
}

ssize_t Socket_Chain_AES::partialRead(void *data, const size_t &datalen)
{
    if (!m_initialized)
        return Socket_Stream::partialRead(data,datalen);

    ssize_t r = Socket_Stream::partialRead(data,datalen);
    if (r<=0)
        return r;

    // Enlarge the buffer to decrypt the requested string...
    while (m_readParams.aesBlock_curSize<(size_t)r)
    {
        regenIV(&m_readParams);
        if (!appendNewAESBlock( &m_readParams, m_readParams.handshake.phase2Key, m_readParams.currentIV))
            return false;
    }
    // Decrypt the data...
    m_readParams.cryptoXOR((char *)data,r);
    return r;
}

ssize_t Socket_Chain_AES::partialWrite(const void *data, const size_t &datalen)
{
    if (!m_initialized)
        return Socket_Stream::partialWrite(data,datalen);

    // Copy the data...
    char * edata = new char [datalen];
    memcpy(edata,data,datalen);

    // Enlarge the buffer to decrypt the requested string...
    while (m_writeParams.aesBlock_curSize<datalen)
    {
        regenIV(&m_writeParams);
        if (!appendNewAESBlock( &m_writeParams, m_writeParams.handshake.phase2Key, m_writeParams.currentIV))
        {
            delete [] edata;
            return false;
        }
    }

    // Encrypt the data...
    m_writeParams.cryptoXOR(edata,datalen,true);

    // Try to transmit the encrypted data...
    ssize_t r=Socket_Stream::partialWrite(edata,datalen);
    if (r>0)
    {
        // Data transmited.. reduce it.
        m_writeParams.reduce(r);
    }

    // destroy this data..
    memset(edata,0,datalen);
    delete [] edata;
    return r;
}

bool Socket_Chain_AES::postAcceptSubInitialization()
{
    char *p1,*p2;

    ///////////////////////////////////////////////
    // Transmition:
    ///////////////////////////////////////////////
    // Put the very first IV
    genRandomBytes(m_writeParams.handShakeIV,sizeof(m_writeParams.handShakeIV));
    // Transmit the very first IV
    if (!writeFull(m_writeParams.handShakeIV,sizeof(m_writeParams.handShakeIV)))
        return false;
    // Create the en/decryption xoring block
    if (!appendNewAESBlock( &m_writeParams, m_phase1Key, m_writeParams.handShakeIV))
        return false;
    // Create the next local (write) keys...
    genRandomBytes(m_writeParams.handshake.phase2Key,sizeof(m_writeParams.handshake.phase2Key));
    genRandomBytes(m_writeParams.handshake.IVSeed,sizeof(m_writeParams.handshake.IVSeed));
    // Create the memory to transmit this...
    char vFirstLoad[sizeof(sHandShakeHeader)];
    memcpy(vFirstLoad,&(m_writeParams.handshake),sizeof(sHandShakeHeader));
    // Encrypt the header.
    m_writeParams.cryptoXOR(vFirstLoad,sizeof(sHandShakeHeader));
    // Transmit the encrypted header.
    if (!writeFull(vFirstLoad,sizeof(sHandShakeHeader)))
        return false;
    // Initialize with the transmited values:
    m_writeParams.cleanAESBlock();

    // Fill the initial Write IV values on MT engine.
    p1 = m_writeParams.handshake.IVSeed+0;
    p2 = m_writeParams.handshake.IVSeed+8;
    m_writeParams.mt19937IV[0].seed(*((uint64_t *)p1));
    m_writeParams.mt19937IV[1].seed(*((uint64_t *)p2));

    ///////////////////////////////////////////////
    // Reception:
    ///////////////////////////////////////////////
    // Read the IV
    if (!readFull(m_readParams.handShakeIV,sizeof(m_readParams.handShakeIV)))
        return false;
    // Read the encryted header
    if (!readFull(vFirstLoad,sizeof(sHandShakeHeader)))
        return false;
    // Create the en/decryption xoring block
    if (!appendNewAESBlock( &m_readParams, m_phase1Key, m_readParams.handShakeIV))
        return false;
    // Decrypt the header.
    m_readParams.cryptoXOR(vFirstLoad,sizeof(sHandShakeHeader));

    sHandShakeHeader * hshdr = (sHandShakeHeader *)vFirstLoad;
    m_readParams.handshake = *hshdr;

    // Check the decryption:
    if (memcmp(m_readParams.handshake.magicBytes,"IHDR",4))
        return false;
    // If the decryption is OK... REGEN the AES block...
    m_readParams.cleanAESBlock();
    // Fill the initial Read IV values on MT engine.
    p1 = m_readParams.handshake.IVSeed+0;
    p2 = m_readParams.handshake.IVSeed+8;
    m_readParams.mt19937IV[0].seed(*((uint64_t *)p1));
    m_readParams.mt19937IV[1].seed(*((uint64_t *)p2));

    // clean the mem...
    ZeroBStruct(vFirstLoad);

    m_initialized = true;
    return true;
}

bool Socket_Chain_AES::postConnectSubInitialization()
{
    return postAcceptSubInitialization();
}

const EVP_CIPHER *Socket_Chain_AES::openSSLInit()
{
    /* Initialise the library */
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    OPENSSL_config(nullptr);
#endif

    return EVP_aes_256_gcm();
}

bool Socket_Chain_AES::appendNewAESBlock(sSideParams *params, const char *key, const char *iv)
{
    // Create the AES block here.
    char * cipherText = new char [m_aesRegenBlockSize*2];

    int len;
    int cipherText_len;

    // Init AES here.
    EVP_CIPHER_CTX *ctx;
    if(!(ctx = EVP_CIPHER_CTX_new()))
    {
        delete [] cipherText;
        return false;
    }

    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, (unsigned char *)key, (unsigned char *)iv))
    {
        EVP_CIPHER_CTX_free(ctx);
        delete [] cipherText;
        return false;
    }

    // Encrypt the plaintext block
    char * plainText = genPlainText();
    if(1 != EVP_EncryptUpdate(ctx, (unsigned char *)cipherText, &len, (unsigned char *)plainText, m_aesRegenBlockSize))
    {
        EVP_CIPHER_CTX_free(ctx);
        delete [] cipherText;
        delete [] plainText;
        return false;
    }
    delete [] plainText;
    cipherText_len = len;
    if(1 != EVP_EncryptFinal_ex(ctx, (unsigned char *)cipherText + len, &len))
    {
        EVP_CIPHER_CTX_free(ctx);
        delete [] cipherText;
        return false;
    }
    cipherText_len += len;

    params->appendAESBlock(cipherText,m_aesRegenBlockSize);

    delete [] cipherText;
    // Now it's encrypted :)
    EVP_CIPHER_CTX_free(ctx);

    return true;
}

void Socket_Chain_AES::genRandomBytes(char *bytes, size_t size)
{
#ifdef _WIN32
    HCRYPTPROV hCryptProv;
    if(CryptAcquireContext(&hCryptProv,nullptr,nullptr,PROV_RSA_FULL,0))
    {
        if(!CryptGenRandom(hCryptProv, size, (BYTE *)bytes))
        {
            genRandomWeakBytes(bytes,size);
        }
        CryptReleaseContext(hCryptProv, 0);
    }
    else
    {
        genRandomWeakBytes(bytes,size);
    }
#else
    FILE * fp = fopen("/dev/random", "rb");
    if (!fp)
    {
        genRandomWeakBytes(bytes,size);
    }
    else
    {
        for (size_t szRead = 0; szRead<size ; )
        {
            size_t curRead = fread(bytes+szRead, size-szRead, 1, fp);
            if (curRead == size-szRead) szRead+=curRead;
            else
            {
                // Problem reading...
                fclose(fp);
                genRandomWeakBytes(bytes,size);
                return;
            }
        }
        fclose(fp);
    }
#endif

}

void Socket_Chain_AES::genRandomWeakBytes(char *bytes, size_t size)
{
    // 256bit entropy is better than nothing... (not guaranteed)
    std::random_device rd;
    std::mt19937_64 eng1(rd());
    std::mt19937_64 eng2(rd());
    std::mt19937_64 eng3(rd());
    std::mt19937_64 eng4(rd());
    std::uniform_int_distribution<unsigned long long> distr;

    char * rptr = bytes;
    int mtv = 0;
    for (size_t x=size; x>0; x-=(x>8?8:x))
    {
        uint64_t r64;
        switch(mtv)
        {
        case 0:
            r64 = distr( eng1 );
            mtv++;
            break;
        case 1:
            r64 = distr( eng2 );
            mtv++;
            break;
        case 2:
            r64 = distr( eng3 );
            mtv++;
            break;
        case 3:
            r64 = distr( eng4 );
            mtv=0;
            break;
        default:
            break;
        }
        memcpy(rptr,&r64, x>=8?8:x);
        rptr+=8;
    }
}

void Socket_Chain_AES::regenIV(sSideParams *param)
{
    // IV regeneration system :)
    std::uniform_int_distribution<unsigned long long> distr;
    uint64_t r64[2];
    r64[0] = distr(param->mt19937IV[0]);
    r64[1] = distr(param->mt19937IV[1]);
    memcpy(param->currentIV+0,&(r64[0]),8);
    memcpy(param->currentIV+8,&(r64[1]),8);
}

char *Socket_Chain_AES::genPlainText()
{
    char * vGen = new char [m_aesRegenBlockSize];
    for (size_t i=0;i<m_aesRegenBlockSize;i++)
    {
        vGen[i] = (i*487)%256;
    }
    return vGen;
}
