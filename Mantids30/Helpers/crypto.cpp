#include "crypto.h"
#include "encoders.h"
#include "random.h"

#include <cstring>
#include <openssl/evp.h>
#include <openssl/ossl_typ.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <optional>
#include <sys/types.h>

#include "safeint.h"

using namespace Mantids30::Helpers;


std::optional<std::string> Crypto::AES256EncryptB64(const unsigned char * input, const size_t & inputLen, const char * key, const size_t & keyLen)
{
    std::string out;
    bool ok = false;

    // Create the random salt (128bit) and derived Key (256bit)...
    uint8_t salt[128/8],derivedKey[256/8];

    Helpers::Random::createRandomSalt128(salt);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int err;

    if (!ctx)
        return out;

    // Derive the key...
    if (PKCS5_PBKDF2_HMAC( key , safeStaticCast<uint32_t,size_t>(keyLen), salt, sizeof(salt), 100000, EVP_sha256(), sizeof(derivedKey), derivedKey  ) == 1)
    {
        // Initialize the encryption...
        if ((err = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, derivedKey, salt)) == 1)
        {
            if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL))
            {
                int len;
                size_t cipherOutLength = 32;
                uint8_t* cipherOutText = new uint8_t[(inputLen * 2)+32];

                memcpy(cipherOutText,salt,16);
                if (EVP_EncryptUpdate(ctx, cipherOutText + cipherOutLength, &len, input, inputLen) == 1 && len>=0)
                {
                    cipherOutLength += len;
                    if (EVP_EncryptFinal_ex(ctx, cipherOutText + cipherOutLength, static_cast<int*>(&len)) == 1 && len>=0)
                    {
                        cipherOutLength += len;
                        std::uint8_t gcmTag[16];

                        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, gcmTag))
                        {
                            memcpy(cipherOutText+16,gcmTag,16);
                            out = Helpers::Encoders::encodeToBase64( cipherOutText, cipherOutLength );
                            ok = true;
                        }
                    }
                }
                delete[] cipherOutText;
            }
        }
    }
    EVP_CIPHER_CTX_free(ctx);

    if (!ok)
        return std::nullopt;

    return out;
}

std::optional<std::string> Crypto::AES256EncryptB64(const std::string &input, const char *key, const size_t & keyLen)
{
    return AES256EncryptB64(reinterpret_cast<const unsigned char *>(input.c_str()),input.length(),key,keyLen);
}

std::optional<std::string> Crypto::AES256EncryptB64(const std::string &input,  const std::string &key)
{
    return AES256EncryptB64(reinterpret_cast<const unsigned char *>(input.c_str()),input.length(),key.c_str(),key.length());
}

std::shared_ptr<Mem::BinaryDataContainer> Crypto::AES256DecryptB64ToBin(
    const std::string &input, const char *key, const size_t &keyLen)
{
    bool ok;

    auto r = std::make_shared<Mem::BinaryDataContainer>( input.length() );

    if (!r->data)
        return r;

    auto dec = Helpers::Encoders::decodeFromBase64ToBin(input);

    if (dec->data && dec->cur>=32)
    {
        uint8_t salt[128/8],derivedKey[256/8],gcmTag[16];
        // unsigned char * encryptedData = (static_cast<unsigned char *>(dec->data))+32;
        // uint64_t encryptedDataLen;
        memcpy(salt,dec->data,16);
        memcpy(gcmTag,(static_cast<unsigned char *>(dec->data))+16,16);

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        int err;

        if (!ctx)
            return r;

        // Derive the key...
        if (PKCS5_PBKDF2_HMAC( key , safeStaticCast<uint32_t,size_t>(keyLen), salt, sizeof(salt), 100000, EVP_sha256(), sizeof(derivedKey), derivedKey  ) == 1)
        {
            // Initialize the encryption...
            if ((err = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, derivedKey, salt)) == 1)
            {
                if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL))
                {
                    int len=-1;
                    if (EVP_DecryptUpdate(ctx, (unsigned char *) r->data, &len,  static_cast<unsigned char *>(dec->data)+32, dec->cur-32 ) == 1 && len>=0)
                    {
                        r->cur+=len;

                        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, (void*)gcmTag))
                        {
                            if ((err = EVP_DecryptFinal_ex(ctx, static_cast<unsigned char *>(r->data) + r->cur, &len)) == 1 && len>=0)
                            {
                                r->cur += len;

                                // OK...
                                ok = false;
                            }
                        }
                    }
                    else
                    {
                        int o=0;
                        o++;
                    }
                }
            }
        }
        EVP_CIPHER_CTX_free(ctx);
    }

    if (ok)
        return r;
    return nullptr;
}


std::optional<std::string> Crypto::AES256DecryptB64(const std::string &input,  const char * key, const size_t & keyLen)
{
    if (auto i = AES256DecryptB64ToBin(input,key,keyLen))
    {
        return i->toString();
    }
    return std::nullopt;
}
std::optional<std::string> Crypto::AES256DecryptB64(const std::string &input,  const std::string & key)
{
    if (auto i=AES256DecryptB64ToBin(input,key.c_str(),key.length()))
    {
        return i->toString();
    }
    return std::nullopt;
}

std::string Crypto::calcSHA1(const std::string &password)
{
    std::string r;
    unsigned char buffer_sha2[SHA_DIGEST_LENGTH+1];
    SHA_CTX sha1;
    SHA1_Init (&sha1);
    SHA1_Update (&sha1, (const unsigned char *) password.c_str(), password.length());
    SHA1_Final ( buffer_sha2, &sha1);
    return Encoders::toHex(buffer_sha2,SHA_DIGEST_LENGTH);
}

std::string Crypto::calcSHA256(const std::string &password)
{
    std::string r;
    unsigned char buffer_sha2[SHA256_DIGEST_LENGTH+1];
    SHA256_CTX sha2;
    SHA256_Init (&sha2);
    SHA256_Update (&sha2, (const unsigned char *) password.c_str(), password.length());
    SHA256_Final ( buffer_sha2, &sha2);
    return Encoders::toHex(buffer_sha2,SHA256_DIGEST_LENGTH);
}

std::string Crypto::calcSHA512(const std::string &password)
{
    std::string r;
    unsigned char buffer_sha2[SHA512_DIGEST_LENGTH+1];
    SHA512_CTX sha2;
    SHA512_Init (&sha2);
    SHA512_Update (&sha2, (const unsigned char *) password.c_str(), password.length());
    SHA512_Final ( buffer_sha2, &sha2);
    return Encoders::toHex(buffer_sha2,SHA512_DIGEST_LENGTH);
}

std::string Crypto::calcSSHA256(const std::string &password, const unsigned char * ssalt)
{
    std::string r;
    unsigned char buffer_sha2[SHA256_DIGEST_LENGTH+1];
    SHA256_CTX sha2;
    SHA256_Init (&sha2);
    SHA256_Update (&sha2, (const unsigned char *) password.c_str(), password.length());
    SHA256_Update (&sha2, ssalt, 4);
    SHA256_Final ( buffer_sha2, &sha2);
    return Encoders::toHex(buffer_sha2,SHA256_DIGEST_LENGTH);
}

std::string Crypto::calcSSHA512(const std::string &password, const unsigned char * ssalt)
{
    std::string r;
    unsigned char buffer_sha2[SHA512_DIGEST_LENGTH+1];
    SHA512_CTX sha2;
    SHA512_Init (&sha2);
    SHA512_Update (&sha2, (const unsigned char *) password.c_str(), password.length());
    SHA512_Update (&sha2, ssalt, 4);
    SHA512_Final ( buffer_sha2, &sha2);
    return Encoders::toHex(buffer_sha2,SHA512_DIGEST_LENGTH);
}
