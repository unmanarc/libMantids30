#include "jwt.h"
#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/Helpers/json.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>

using namespace Mantids30::DataFormat;

std::string JWT::createHeader() {
    Json::Value header;
    header["typ"] = "JWT";

    switch (m_algorithm) {
    case Algorithm::HS256:
        header["alg"] = "HS256";
        break;
    case Algorithm::HS384:
        header["alg"] = "HS384";
        break;
    case Algorithm::HS512:
        header["alg"] = "HS512";
        break;
    case Algorithm::RS256:
        header["alg"] = "RS256";
        break;
    case Algorithm::RS384:
        header["alg"] = "RS384";
        break;
    case Algorithm::RS512:
        header["alg"] = "RS512";
        break;
    }

    Json::StreamWriterBuilder writer;
    std::string header_str = Json::writeString(writer, header);

    return Helpers::Encoders::encodeToBase64(header_str, true);
}

bool JWT::isAlgorithmSupported(const std::string &algorithm)
{
    if (algorithm == "HS256")
        return true;
    if (algorithm == "RS256")
        return true;

    if (algorithm == "HS384")
        return true;
    if (algorithm == "RS384")
        return true;

    if (algorithm == "HS512")
        return true;
    if (algorithm == "RS512")
        return true;

    return false;
}

std::shared_ptr<JWT::RAWSignature> JWT::createSignature(const std::string& data) {

    std::shared_ptr<JWT::RAWSignature> r;

    AlgorithmDetails algorithmDetails(m_algorithm);

    if (algorithmDetails.isUsingHMAC)
    {
        r = createHMACSignature(algorithmDetails.nid,data);
    }
    else if (algorithmDetails.usingRSA)
    {
        r = createRSASignature(algorithmDetails.nid,data);
    }

    return r;
}

std::shared_ptr<JWT::RAWSignature> JWT::createHMACSignature(int hashType, const std::string &data)
{
    std::shared_ptr<JWT::RAWSignature> r;
    r.reset(new JWT::RAWSignature);

    r->m_digestSize = EVP_MAX_MD_SIZE + 1;
    r->m_digest = new unsigned char [r->m_digestSize];

    if (!m_sharedSecret.empty() && r->m_digest)
    {
        HMAC(   EVP_get_digestbynid(hashType),
                m_sharedSecret.data(),
                m_sharedSecret.size(),
                reinterpret_cast<const unsigned char*>(data.data()),
                data.size(),
                r->m_digest,
                &(r->m_digestSize)
                );
        r->m_result = RAWSignature::SIG_OK;
        return r;
    }
    r->m_result = RAWSignature::SIG_EMPTY_KEY;
    return r;
}

std::shared_ptr<JWT::RAWSignature> JWT::createRSASignature(int hashType, const std::string &data)
{
    std::shared_ptr<JWT::RAWSignature> r;
    r.reset(new JWT::RAWSignature);

    r->m_result = RAWSignature::SIG_OK;
    EVP_PKEY* pkey = nullptr;
    RSA* rsa = nullptr;
    BIO* bio = nullptr;
    size_t sig_len = 0;

    // Read the private key from a string
    bio = BIO_new_mem_buf(m_privateSecret.data(), m_privateSecret.size());
    if (bio)
    {
        pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);
        if (pkey)
        {
            EVP_MD_CTX* mdctx = EVP_MD_CTX_new();

            if (mdctx)
            {

                if (EVP_DigestSignInit(mdctx, NULL, EVP_get_digestbynid(hashType), NULL, pkey) == 1)
                {
                    EVP_DigestSignUpdate(mdctx, data.c_str(), data.length());
                    size_t signature_len = 0;
                    EVP_DigestSignFinal(mdctx, NULL, &signature_len);
                    r->m_digestSize = signature_len;

                    if (r->m_digestSize>0)
                    {
                        r->m_digest = new unsigned char [r->m_digestSize];
                        if (r->m_digest)
                        {
                            EVP_DigestSignFinal(mdctx, r->m_digest, &signature_len);
                            //  r->m_digestSize = signature_len;
                            r->m_result = RAWSignature::SIG_OK;
                        }
                        else
                            r->m_result = RAWSignature::SIG_ERROR_CREATING_SIGNATURE;
                    }
                    else
                        r->m_result = RAWSignature::SIG_ERROR_CREATING_SIGNATURE;
                }
                else
                    r->m_result = RAWSignature::SIG_ERROR_CREATING_SIGNATURE;


                EVP_MD_CTX_free(mdctx);
            }
            else
            {
                r->m_result = RAWSignature::SIG_ERROR_CREATING_SIGNATURE;
            }

            EVP_PKEY_free(pkey);
        }
        else
        {
            // Error reading the private key
            r->m_result = RAWSignature::SIG_ERROR_READING_KEY;
        }
    }

    return r;
}

int JWT::validateRSASignature(int hashType, const std::string &data, const char *signature, unsigned int signatureLength)
{
    int error = 0;
    EVP_PKEY* pkey = nullptr;
    RSA* rsa = nullptr;
    BIO* bio = nullptr;

    // Read the public key from a string
    bio = BIO_new_mem_buf(m_publicSecret.data(), m_publicSecret.size());
    if (bio)
    {
        pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);
        if (pkey)
        {
            // Verificar el JWT utilizando RS256
            EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
            if (mdctx)
            {

                EVP_DigestVerifyInit(mdctx, NULL, EVP_get_digestbynid(hashType), NULL, pkey);
                EVP_DigestVerifyUpdate(mdctx, data.c_str(), data.length());

                // TODO: signatureLength can be greater and don't affect the validation?
                error = EVP_DigestVerifyFinal(mdctx, reinterpret_cast<const unsigned char*>(signature), signatureLength) == 1 ? 0 : -1;

                // Liberar recursos
                EVP_MD_CTX_free(mdctx);
            }

            EVP_PKEY_free(pkey);
        }
        else
        {
            // Error reading the public key
            error = -3;
        }
    }
    return error;
}

int JWT::getHashTypeNumber()
{
    int hashType = 0;
    switch (m_algorithm) {
    case Algorithm::HS256:
        hashType = NID_sha256;
        break;
    case Algorithm::HS384:
        hashType = NID_sha384;
        break;
    case Algorithm::HS512:
        hashType = NID_sha512;
        break;
    case Algorithm::RS256:
        hashType = NID_sha256;
        break;
    case Algorithm::RS384:
        hashType = NID_sha384;
        break;
    case Algorithm::RS512:
        hashType = NID_sha512;
        break;
    }
    return hashType;
}

std::time_t JWT::defaultMaxTimeBeforeInSeconds() const
{
    return m_defaultMaxTimeBeforeInSeconds;
}

std::time_t JWT::defaultExpirationTimeInSeconds() const
{
    return m_defaultExpirationTimeInSeconds;
}

void JWT::setDefaultMaxTimeBeforeInSeconds(std::time_t newMaxTimeBeforeInSeconds)
{
    m_defaultMaxTimeBeforeInSeconds = newMaxTimeBeforeInSeconds;
}

void JWT::setDefaultMaxExpirationTimeInSeconds(std::time_t newMaxExpirationTimeInSeconds)
{
    m_defaultExpirationTimeInSeconds = newMaxExpirationTimeInSeconds;
}

void JWT::setPrivateSecret(const std::string &newPrivateSecret)
{
    m_privateSecret = newPrivateSecret;
}

bool JWT::isHMACAlgorithm(const std::string &algorithm)
{
    if (algorithm == "HS256")
        return true;
    if (algorithm == "HS384")
        return true;
    if (algorithm == "HS512")
        return true;
    return false;
}

bool JWT::isRSAAlgorithm(const std::string &algorithm)
{
    if (algorithm == "RS256")
        return true;
    if (algorithm == "RS384")
        return true;
    if (algorithm == "RS512")
        return true;
    return false;
}

void JWT::setPublicSecret(const std::string &newPublicSecret)
{
    m_publicSecret = newPublicSecret;
    m_cache.clear();
}

void JWT::setSharedSecret(const std::string &newSharedSecret)
{
    m_sharedSecret = newSharedSecret;
    m_cache.clear();
}

std::string JWT::sign(const Json::Value& payload)
{
    Json::StreamWriterBuilder writer;
    std::string payload_str = Json::writeString(writer, payload);
    std::string header_str = createHeader();
    std::string signature;

    auto eSignature = createSignature(header_str + '.' + Helpers::Encoders::encodeToBase64(payload_str, true));

    if (eSignature->m_result == RAWSignature::SIG_OK)
        signature = Helpers::Encoders::encodeToBase64(eSignature->m_digest,eSignature->m_digestSize, true);

    return header_str + '.' + Helpers::Encoders::encodeToBase64(payload_str, true) + '.' + signature;
}

std::string JWT::signFromToken(Token &token, bool updateDefaultTimeValues)
{
    if (updateDefaultTimeValues)
    {
        token.setExpirationTime( time(nullptr) + m_defaultExpirationTimeInSeconds );
        token.setIssuedAt( time(nullptr) );
        token.setNotBefore( time(nullptr) - m_defaultMaxTimeBeforeInSeconds );
    }

    return sign(*(token.getClaimsPTR()));
}


bool JWT::verify(const std::string &fullSignedToken, JWT::Token *tokenPayloadOutput)
{
    JWT::Token dummyToken;
    if (!tokenPayloadOutput)
        tokenPayloadOutput = &dummyToken;

    // Find the positions of the '.' characters that separate the header, payload, and signature
    size_t pos_header = fullSignedToken.find('.');
    size_t pos_payload = fullSignedToken.rfind('.');

    // If any of the '.' characters are not found or are in the wrong order, return false
    if (pos_header == std::string::npos || pos_payload == std::string::npos || pos_header >= pos_payload)
    {
        return false;
    }

    // Extract the base64-encoded header, payload, and signature substrings from the token
    std::string header_b64 = fullSignedToken.substr(0, pos_header);
    std::string payload_b64 = fullSignedToken.substr(pos_header + 1, pos_payload - pos_header - 1);
    std::string signature_b64 = fullSignedToken.substr(pos_payload + 1);

    // Decode the base64-encoded header, payload, and signature strings
    std::string header_str = Helpers::Encoders::decodeFromBase64(header_b64, true);
    std::string payload_str = Helpers::Encoders::decodeFromBase64(payload_b64, true);
    std::string signature_str = Helpers::Encoders::decodeFromBase64(signature_b64, true);


    // If we have a backchannel to check the token itself, check trough backchannel.
    if (verificationCallback)
    {
        if (verificationCallback(fullSignedToken))
        {
            tokenPayloadOutput->decodePayload(payload_str);
            tokenPayloadOutput->setSignatureVerified(true);

            return tokenPayloadOutput->isValid();
        }
        else
        {
            // Failed to validate remotely, don't continue locally.
            return false;
        }
    }

    // Check if the token is already in the cache
    if (m_cache.checkToken(payload_str))
    {
        tokenPayloadOutput->decodePayload(payload_str);
        tokenPayloadOutput->setSignatureVerified(true);

        // Return if verified and not expired.
        return tokenPayloadOutput->isValid();
    }

    // Parse the JSON header using the JsonCpp library
    Json::CharReaderBuilder reader;
    std::unique_ptr<Json::CharReader> charReader(reader.newCharReader()); // create a unique_ptr to manage the JsonCpp char reader
    Json::Value header_json;
    std::string errs;

    if (!charReader->parse(header_str.data(), header_str.data() + header_str.size(), &header_json, &errs))
    {
        // If the header cannot be parsed, return false
        return false;
    }

    bool isSignatureVerified = false;

    // Check that the header algorithm is supported
    auto incomingAlgorithm = JSON_ASSTRING(header_json, "alg", "");
    if (!isAlgorithmSupported(incomingAlgorithm))
    {
        return false;
    }

    if (isHMACAlgorithm(incomingAlgorithm))
    {
        // Create the signature using the header and payload, and compare with the decoded signature
        auto computed_signature = createSignature(header_b64 + '.' + payload_b64);
        if (computed_signature->m_result != RAWSignature::SIG_OK)
            return false;

        isSignatureVerified = false;

        if (computed_signature->m_digestSize > 0 && computed_signature->m_digestSize == signature_str.size())
        {
            isSignatureVerified = memcmp(computed_signature->m_digest, signature_str.data(), computed_signature->m_digestSize) == 0;
        }
    }
    else if (isRSAAlgorithm(incomingAlgorithm))
    {
        // Create the signature using the header and payload, and compare with the decoded signature

        // TODO: return specific problems...
        isSignatureVerified = validateRSASignature(getHashTypeNumber(), header_b64 + '.' + payload_b64, signature_str.data(), signature_str.size()) == 0;
    }

    if (isSignatureVerified)
    {
        tokenPayloadOutput->decodePayload(payload_str);
        m_cache.add(payload_str);
    }
    tokenPayloadOutput->setSignatureVerified(isSignatureVerified);
    tokenPayloadOutput->setRevoked(m_revocation.isSignatureRevoked(signature_str));

    // TODO: indicate the error (invalid signature, revoked signature, etc)
    return tokenPayloadOutput->isValid();
}

bool JWT::decodeNoVerify(const std::string &fullSignedToken, Token *tokenPayloadOutput)
{
    // Find the positions of the '.' characters that separate the header, payload, and signature
    size_t pos_header = fullSignedToken.find('.');
    size_t pos_payload = fullSignedToken.rfind('.');

    // If any of the '.' characters are not found or are in the wrong order, return false
    if (pos_header == std::string::npos || pos_payload == std::string::npos || pos_header >= pos_payload)
    {
        return false;
    }

    // Extract the base64-encoded header, payload, and signature substrings from the token
    std::string header_b64 = fullSignedToken.substr(0, pos_header);
    std::string payload_b64 = fullSignedToken.substr(pos_header + 1, pos_payload - pos_header - 1);
    //std::string signature_b64 = fullSignedToken.substr(pos_payload + 1);

    // Decode the base64-encoded header, payload, and signature strings
    std::string header_str = Helpers::Encoders::decodeFromBase64(header_b64, true);
    std::string payload_str = Helpers::Encoders::decodeFromBase64(payload_b64, true);
    //std::string signature_str = Helpers::Encoders::decodeFromBase64(signature_b64, true);

    return tokenPayloadOutput->decodePayload(payload_str);
}

JWT::Token JWT::verifyAndDecodeTokenPayload(const std::string &fullSignedToken)
{
    JWT::Token r,empty;
    if (verify(fullSignedToken, &r))
        return r;
    else
        return empty;
}

